//
// Created by brian on 11/22/23.
//

#include "hound/common/util.hpp"
#include "hound/common/global.hpp"
#include "hound/live_parser.hpp"

using namespace hd::global;

hd::type::LiveParser::LiveParser() {
  util::OpenLiveHandle(opt, mHandle);
  mConsumerTasks.reserve(opt.workers);
  for (int i = 0; i < opt.workers; ++i) {
    mConsumerTasks.emplace_back(&LiveParser::consumer_job, this);
  }
}

void hd::type::LiveParser::startCapture() {
  using namespace hd::global;
  if (opt.duration > 0) {
    /// canceler thread
    std::thread([this] {
      std::this_thread::sleep_for(Seconds(opt.duration));
      this->stopCapture();
    }).detach();
  }
  pcap_loop(mHandle.get(), opt.num_packets, liveHandler, reinterpret_cast<u_char*>(this));
}

void hd::type::LiveParser::liveHandler(u_char* user_data, const pcap_pkthdr* pkthdr, const u_char* packet) {
  auto const _this{reinterpret_cast<LiveParser*>(user_data)};
  _this->doubleBufferQueue.enqueue({pkthdr, packet, std::min(opt.total_bytes, pkthdr->caplen)});
}

void hd::type::LiveParser::consumer_job() {
  sink::KafkaSink sink;
  while (is_running) {
    std::this_thread::sleep_for(Seconds(opt.wait1));
    std::shared_ptr current_queue = doubleBufferQueue.read();
    if (current_queue->size_approx() == 0) {
      std::this_thread::sleep_for(Seconds(opt.wait2));
      continue;
    }
    mTaskExecutor.AddTask([current_queue, &sink] { sink.MakeFlow(*current_queue); });
  }
}

void hd::type::LiveParser::stopCapture() {
  pcap_breakloop(mHandle.get());
  is_running = false;
  easylog::set_console(true);
  EINFO(ELOG_INFO << YELLOW("正在处理剩下的数据"));
}

hd::type::LiveParser::~LiveParser() {
  is_running = false;
  for (auto& item : mConsumerTasks) item.join();
#if defined(BENCHMARK)
  using namespace global;
  std::printf("%s%d\n", CYAN("num_captured_packet = "), num_captured_packet.load());
  std::printf("%s%d\n", CYAN("num_dropped_packets = "), num_dropped_packets.load());
  std::printf("%s%d\n", CYAN("num_consumed_packet = "), num_consumed_packet.load());
  std::printf("%s%d\n", CYAN("num_written_csv = "), num_written_csv.load());
#endif //- #if defined(BENCHMARK)
  EDEBUG(ELOG_DEBUG << CYAN("处理完成， raw包队列剩余 ") << doubleBufferQueue.size());
}

bool hd::type::LiveParser::isRunning() const {
  return is_running;
}
