//
// Created by brian on 11/22/23.
//

#include "hound/parser/live_parser.hpp"
#include "hound/common/global.hpp"
#include "hound/common/util.hpp"
#include "hound/sink/kafka/kafka_util.hpp"

using namespace hd::global;

hd::type::LiveParser::LiveParser() {
  conn_conf.read_kafka_conf(opt.kafka_config);
  hd::util::InitGetConf(conn_conf, _serverConf, _topicConf);
  ELOG_DEBUG << "初始化连接配置";
  util::OpenLiveHandle(opt, mHandle);
  mConsumerTasks.reserve(opt.workers);
  for (int i = 0; i < opt.workers; ++i) {
    mConsumerTasks.emplace_back(std::thread(&LiveParser::consumer_job, this));
  }
}

void hd::type::LiveParser::startCapture() {
  using namespace hd::global;
  if (opt.duration > 0) {
    /// canceler thread
    std::thread([this] {
      std::this_thread::sleep_for(std::chrono::seconds(opt.duration));
      this->stopCapture();
    }).detach();
  }
  pcap_loop(mHandle.get(), opt.num_packets, liveHandler, reinterpret_cast<u_char*>(this));
  cv_consumer.notify_all();
}

void hd::type::LiveParser::liveHandler(u_char* user_data, const pcap_pkthdr* pkthdr, const u_char* packet) {
  auto const _this{reinterpret_cast<LiveParser*>(user_data)};
  std::unique_lock _accessToQueue(_this->mQueueLock);
  _this->mPacketQueue.emplace(pkthdr, packet, std::min(opt.payload + 120, static_cast<int>(pkthdr->caplen)));
  _this->cv_consumer.notify_all();
  _accessToQueue.unlock();
#if defined(BENCHMARK)
  ++num_captured_packet;
#endif // BENCHMARK
}

void hd::type::LiveParser::consumer_job() {
  hd::sink::KafkaSink sink(conn_conf, _serverConf, _topicConf);
  ELOG_DEBUG << "创建 KafkaSink";
  /// 采用标志变量keepRunning来控制detach的线程
  while (is_running) {
    std::unique_lock lock(this->mQueueLock);
    this->cv_consumer.wait(lock, [this] { return not this->mPacketQueue.empty() or not is_running; });
    if (not is_running) break;
    if (this->mPacketQueue.empty()) continue;
    //TODO: mPacketQueue->vector, mPacketQueue.swap()
    auto front{mPacketQueue.front()};
    this->mPacketQueue.pop();
    cv_producer.notify_all();
    lock.unlock();
    sink.consume_data(front);
#if defined(BENCHMARK)
    ++num_consumed_packet;
#endif // defined(BENCHMARK)
  }
  ELOG_INFO << YELLOW("发送消息任务 [") << std::this_thread::get_id() << YELLOW("] 结束");
}

void hd::type::LiveParser::stopCapture() {
  pcap_breakloop(mHandle.get());
  is_running = false;
  cv_consumer.notify_all();
  ELOG_INFO << CYAN("准备退出.., 不过要先处理剩下的数据包....");
}

hd::type::LiveParser::~LiveParser() {
  using namespace std::chrono_literals;
  for (std::thread& item : mConsumerTasks) {
    item.detach();
  }
  /// 先等待worker线程消费队列直至为空
  while (not mPacketQueue.empty()) {
    std::this_thread::sleep_for(10ms);
  }
  is_running = false;
  cv_consumer.notify_all();
#if defined(BENCHMARK)
  using namespace global;
  std::printf("%s%d\n", CYAN("num_captured_packet = "), num_captured_packet.load());
  std::printf("%s%d\n", CYAN("num_dropped_packets = "), num_dropped_packets.load());
  std::printf("%s%d\n", CYAN("num_consumed_packet = "), num_consumed_packet.load());
  std::printf("%s%d\n", CYAN("num_written_csv = "), num_written_csv.load());
#endif //- #if defined(BENCHMARK)
  ELOG_DEBUG << CYAN("处理完成， raw包队列剩余 ") << mPacketQueue.size();
}
