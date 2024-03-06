//
// Created by brian on 11/22/23.
//

#ifndef FC_REFACTORED_LIVE_PARSER_HPP
#define FC_REFACTORED_LIVE_PARSER_HPP

#include <pcap/pcap.h>
#include <atomic>
#include <hound/type/raw_packet_info.hpp>
#include <condition_variable>
#include <hound/sink/rpc_sink.hpp>
// #include <hound/sink/console_sink.hpp>
#include <hound/type/deleters.hpp>
#include <hound/sink/json_file_sink.hpp>

namespace hd::type {
class LiveParser {
public:
  explicit LiveParser();

  void startCapture();

  void stopCapture();

  ~LiveParser();

public:
  std::atomic<bool> is_running{true};

private:
  static void liveHandler(u_char*, const pcap_pkthdr*, const u_char*);

  void consumer_job();

private:
  pcap_t* mHandle{nullptr};
  std::queue<raw_packet_info> mPacketQueue;

  RpcSink server;
  std::condition_variable cv_producer;      // 生产者条件变量
  std::condition_variable cv_consumer;      // 消费者条件变量

  mutable std::mutex mQueueLock;
};
} // entity

#endif //FC_REFACTORED_LIVE_PARSER_HPP
