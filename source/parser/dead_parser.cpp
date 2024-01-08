//
// Created by brian on 11/22/23.
//
#include <hound/common/util.hpp>
#include <hound/common/macro.hpp>
#include <hound/parser/dead_parser.hpp>
#include <hound/sink/impl/text_file_sink.hpp>


hd::type::DeadParser::DeadParser() {
  this->timer = std::make_unique<Timer>(_timeConsumption_ms_s1, _timeConsumption_ms_s2);
  this->mHandle = util::OpenDeadHandle(global::opt, this->mLinkType);
  if (global::opt.output_file.empty()) {
    mSink.reset(new BaseSink(global::opt.output_file));
    return;
  }
  mSink.reset(new TextFileSink(global::opt.output_file));
}

void hd::type::DeadParser::processFile() {
  using namespace hd::global;
  for (int i = 0; i < opt.workers; ++i) {
    std::thread(&DeadParser::consumer_job, this).detach();
  }
  timer->start();
  pcap_loop(mHandle, opt.num_packets, deadHandler, reinterpret_cast<byte_t*>(this));
  timer->stop1();
}

void hd::type::DeadParser::deadHandler(byte_t* user_data, const pcap_pkthdr* pkthdr, const byte_t* packet) {
  auto const _this{reinterpret_cast<DeadParser*>(user_data)};
  std::unique_lock _accessToQueue(_this->mQueueLock);
  _this->mPacketQueue.push(
    {pkthdr, packet, util::min<int>(global::opt.payload + 128, static_cast<int>(pkthdr->caplen))});
  _accessToQueue.unlock();
  _this->cv_consumer.notify_all();
  ++global::num_captured_packet;
}

void hd::type::DeadParser::consumer_job() {
  /// 采用标志变量keepRunning来控制detach的线程
  while (keepRunning) {
    std::unique_lock lock(this->mQueueLock);
    this->cv_consumer.wait(lock, [this] {
      return not this->mPacketQueue.empty() or not keepRunning;
    });
    if (not keepRunning) break;
    if (this->mPacketQueue.empty()) continue;
    raw_packet_info packetInfo{this->mPacketQueue.front()};
    this->mPacketQueue.pop();
    lock.unlock();
    {
      std::scoped_lock ___a(mProdLock);
      cv_producer.notify_one();
    }
    mSink->consumeData({packetInfo});
    ++global::num_consumed_packet;
  }
}

hd::type::DeadParser::~DeadParser() {
  /// 先等待游离worker线程消费队列直至为空
  while (not this->mPacketQueue.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  /// 再控制游离线程停止访问主线程的资源
  keepRunning.store(false);
  cv_consumer.notify_all();
  using namespace global;
  timer->stop2();
  hd_line(CYAN("num_captured_packet = "), num_captured_packet.load());
  hd_line(CYAN("num_dropped_packets = "), num_dropped_packets.load());
  hd_line(CYAN("num_consumed_packet = "), num_consumed_packet.load());
  hd_line(CYAN("num_written_csv = "), num_written_csv.load());
  hd_debug(this->mPacketQueue.size());
  std::cout << "File Name: " << opt.pcap_file
    << ", Packet Count: " << num_consumed_packet.load()
    << ", Time Consumption1: " << _timeConsumption_ms_s1 << " ms"
    << ", Time Consumption2: " << _timeConsumption_ms_s2 << " ms"
    << std::endl;
  /// 不要强制exit(0), 因为还有worker在死等。
  // exit(EXIT_SUCCESS);
}
