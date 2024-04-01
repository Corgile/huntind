//
// Created by brian on 11/22/23.
//

#ifndef FC_REFACTORED_LIVE_PARSER_HPP
#define FC_REFACTORED_LIVE_PARSER_HPP

#include <pcap/pcap.h>
#include <atomic>
#include <condition_variable>
#include <hound/common/macro.hpp>
#include <hound/type/raw_packet.hpp>
#include <hound/type/deleters.hpp>
#include <hound/sink/kafka_sink.hpp>

namespace hd::type {
class LiveParser {
public:
  LiveParser();

  void startCapture();

  void stopCapture();

  ~LiveParser();

public:
  std::atomic<bool> is_running{true};

private:
  static void liveHandler(u_char*, const pcap_pkthdr*, const u_char*);

  void consumer_job();

private:
  pcap_handle_t mHandle{nullptr};
  raw_list mPacketQueue;
  std::condition_variable cv_producer;      // 生产者条件变量
  std::condition_variable cv_consumer;      // 消费者条件变量
  std::vector<std::thread> mConsumerTasks;
  mutable std::mutex mQueueLock;
};
} // type

#endif //FC_REFACTORED_LIVE_PARSER_HPP
