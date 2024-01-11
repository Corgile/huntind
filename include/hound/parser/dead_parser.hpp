//
// Created by brian on 11/22/23.
//

#ifndef FC_REFACTORED_DEAD_PARSER_HPP
#define FC_REFACTORED_DEAD_PARSER_HPP

#include <pcap/pcap.h>
#include <condition_variable>
#include <hound/type/raw_packet_info.hpp>
#include <hound/sink/base_sink.hpp>

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
  std::queue<raw_packet> mPacketQueue;
  std::atomic<bool> keepRunning{true};
  std::shared_ptr<BaseSink> mSink;
  mutable std::mutex mProdLock;
  std::condition_variable cv_producer; // 生产者条件变量
  std::condition_variable cv_consumer; // 消费者条件变量
  mutable std::mutex mQueueLock;
  mutable std::mutex cvMtx;
  double _timeConsumption_ms_s1 = 0.;
  double _timeConsumption_ms_s2 = 0.;
};
}


#endif //FC_REFACTORED_DEAD_PARSER_HPP
