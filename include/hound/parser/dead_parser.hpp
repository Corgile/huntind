//
// Created by brian on 11/22/23.
//

#ifndef FC_REFACTORED_DEAD_PARSER_HPP
#define FC_REFACTORED_DEAD_PARSER_HPP

#include <pcap/pcap.h>
#include <condition_variable>
#include <hound/type/raw_packet_info.hpp>
#include <hound/sink/base_sink.hpp>
#include <hound/type/timer.hpp>

namespace hd::type {
class DeadParser final {
public:
  DeadParser();

  void processFile();

  ~DeadParser();

private:
  static void deadHandler(u_char*, const pcap_pkthdr*, const u_char*);

  void consumer_job();

private:
  pcap_t* mHandle{nullptr};
  uint32_t mLinkType{};
  std::queue<raw_packet_info> mPacketQueue;
  std::atomic<bool> keepRunning{true};
  std::shared_ptr<BaseSink> mSink;
  mutable std::mutex mProdLock;
  std::condition_variable cv_producer; // 生产者条件变量
  std::condition_variable cv_consumer; // 消费者条件变量
  mutable std::mutex mQueueLock;
  double _timeConsumption_ms_s1 = 0.;
  double _timeConsumption_ms_s2 = 0.;
  std::unique_ptr<Timer> timer;
};
}


#endif //FC_REFACTORED_DEAD_PARSER_HPP
