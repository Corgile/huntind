//
// Created by brian on 11/22/23.
//

#include "hound/live_parser.hpp"

#include <future>

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
  this->mPacketQueue.reserve(150'000);
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
  // TODO lock-free mPacketQueue, now it's vector
  _this->mPacketQueue.emplace_back(pkthdr, packet, std::min(opt.payload + 128, static_cast<int>(pkthdr->caplen)));
  _this->cv_consumer.notify_all();
  _accessToQueue.unlock();
#if defined(BENCHMARK)
  ++num_captured_packet;
#endif // BENCHMARK
}

void hd::type::LiveParser::consumer_job() {
  using namespace std::chrono_literals;
  sink::KafkaSink sink;
  std::list<std::future<void>> tasks;
  while (is_running) {
    for (auto it = tasks.begin(); it != tasks.end();) {
      if (it->wait_for(0s) == std::future_status::ready) {
        it->get();
        it = tasks.erase(it);
      } else {
        ++it;
      }
    }
    std::unique_lock lock(mQueueLock);
    cv_consumer.wait_for(lock, 2s, [this] { return not mPacketQueue.empty(); });
    raw_vector _swapped_buff;
    _swapped_buff.reserve(mPacketQueue.size());
    mPacketQueue.swap(_swapped_buff);
    lock.unlock();
    cv_producer.notify_all();
    if (_swapped_buff.empty()) {
      std::this_thread::sleep_for(5s);
      continue;
    }
    auto shared_data = std::make_shared<raw_vector>(_swapped_buff);
    auto future_task = std::async(std::launch::async,
      [shared_data, &sink]  {
      sink.MakeFlow(shared_data);
    });
    tasks.push_back(std::move(future_task));
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
  cv_consumer.notify_all();
  for (std::thread& item : mConsumerTasks) {
    item.join();
  }
#if defined(BENCHMARK)
  using namespace global;
  std::printf("%s%d\n", CYAN("num_captured_packet = "), num_captured_packet.load());
  std::printf("%s%d\n", CYAN("num_dropped_packets = "), num_dropped_packets.load());
  std::printf("%s%d\n", CYAN("num_consumed_packet = "), num_consumed_packet.load());
  std::printf("%s%d\n", CYAN("num_written_csv = "), num_written_csv.load());
#endif //- #if defined(BENCHMARK)
  easylog::set_console(true);
  ELOG_DEBUG << CYAN("处理完成， raw包队列剩余 ") << mPacketQueue.size();
  easylog::set_console(false);
}

bool hd::type::LiveParser::isRunning() const {
  return is_running;
}
