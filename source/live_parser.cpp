//
// Created by brian on 11/22/23.
//

#include "hound/live_parser.hpp"
#include "hound/common/global.hpp"
#include "hound/common/util.hpp"

using namespace hd::global;

hd::type::LiveParser::LiveParser() {
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
  // cv_consumer.notify_all();
}

void hd::type::LiveParser::liveHandler(u_char* user_data, const pcap_pkthdr* pkthdr, const u_char* packet) {
  auto const _this{reinterpret_cast<LiveParser*>(user_data)};
  _this->mPacketQueue.enqueue({pkthdr, packet, std::min(opt.payload + 128, static_cast<int>(pkthdr->caplen))});
#if defined(BENCHMARK)
  ++num_captured_packet;
#endif // BENCHMARK
}

void hd::type::LiveParser::consumer_job() {
  using namespace std::chrono_literals;
  sink::KafkaSink sink;
  while (is_running) {
    std::this_thread::sleep_for(2s);
    raw_vector _vector;
    {
      std::scoped_lock lock(queue_mtx);
      _vector.swap(mPacketQueue);
    }
    if (_vector.size_approx() == 0) {
      std::this_thread::sleep_for(2s);
      continue;
    }
    auto shared_ = std::make_shared<raw_vector>(std::move(_vector));
    mTaskExecutor.AddTask([shared_, &sink] {
      sink.MakeFlow(*shared_);
    });
  }
}

void hd::type::LiveParser::stopCapture() {
  pcap_breakloop(mHandle.get());
  is_running = false;
  easylog::set_console(true);
  ELOG_INFO << YELLOW("正在处理剩下的数据");
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
  ELOG_DEBUG << CYAN("处理完成， raw包队列剩余 ") << mPacketQueue.size_approx();
}

bool hd::type::LiveParser::isRunning() const {
  return is_running;
}
