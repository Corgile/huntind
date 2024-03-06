//
// Created by brian on 11/22/23.
//

#include <hound/parser/live_parser.hpp>
#include <hound/common/global.hpp>
#include <hound/common/util.hpp>

using namespace hd::global;

hd::type::LiveParser::LiveParser() {
  util::OpenLiveHandle(opt, mHandle);
  for (int i = 0; i < opt.workers; ++i) {
    std::thread(&LiveParser::consumer_job, this).detach();
  }
  if (not opt.output_file.empty()) {
    mSink = std::make_unique<JsonFileSink>(opt.output_file);
  }
  if (opt.kafka_config.empty()) {
    mSink = std::make_unique<BaseSink>(opt.kafka_config);
  } else {
    mSink = std::make_unique<KafkaSink>(opt.kafka_config);
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
}

void hd::type::LiveParser::liveHandler(u_char* user_data, const pcap_pkthdr* pkthdr, const u_char* packet) {
  auto const _this{reinterpret_cast<LiveParser*>(user_data)};
  std::unique_lock _accessToQueue(_this->mQueueLock);
  _this->mPacketQueue.emplace(pkthdr, packet, util::min(opt.payload + 120, static_cast<int>(pkthdr->caplen)));
  _this->cv_consumer.notify_all();
  _accessToQueue.unlock();
#if defined(BENCHMARK)
  ++num_captured_packet;
#endif // BENCHMARK
}

void hd::type::LiveParser::consumer_job() {
  /// 采用标志变量keepRunning来控制detach的线程
  while (is_running) {
    std::unique_lock lock(this->mQueueLock);
    this->cv_consumer.wait(lock, [this] { return not this->mPacketQueue.empty() or not is_running; });
    if (not is_running) break;
    if (this->mPacketQueue.empty()) continue;
    auto front{mPacketQueue.front()};
    this->mPacketQueue.pop();
    cv_producer.notify_all();
    lock.unlock();
    std::ignore = std::async(std::launch::async, &BaseSink::consumeData, mSink.get(), front);
    // mSink->consumeData({front});
#if defined(BENCHMARK)
    ++num_consumed_packet;
#endif // defined(BENCHMARK)
  }
  const std::thread::id this_id = std::this_thread::get_id();
  const auto id_numeric = std::hash<std::thread::id>{}(this_id);
  std::printf("%s%lu%s\n", YELLOW("Worker ["), id_numeric, YELLOW("] 退出"));
}

void hd::type::LiveParser::stopCapture() {
  pcap_breakloop(mHandle.get());
  is_running = false;
  cv_consumer.notify_all();
}

hd::type::LiveParser::~LiveParser() {
  cv_consumer.notify_all();
  using namespace std::chrono_literals;
  /// 先等待游离worker线程消费队列直至为空
  while (not mPacketQueue.empty()) {
    std::this_thread::sleep_for(10ms);
  }
#if defined(BENCHMARK)
  using namespace global;
  std::printf("%s%d\n", CYAN("num_captured_packet = "), num_captured_packet.load());
  std::printf("%s%d\n", CYAN("num_dropped_packets = "), num_dropped_packets.load());
  std::printf("%s%d\n", CYAN("num_consumed_packet = "), num_consumed_packet.load());
  std::printf("%s%d\n", CYAN("num_written_csv = "), num_written_csv.load());
#endif //- #if defined(BENCHMARK)
  hd_debug("raw包队列剩余: ", mPacketQueue.size());
}
