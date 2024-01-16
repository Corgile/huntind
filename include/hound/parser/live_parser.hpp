//
// hound / live_parser.hpp. 
// Created by brian on 2024-01-12.
//

#ifndef LIVE_PARSER_HPP
#define LIVE_PARSER_HPP
#if defined(HD_KAFKA)
#include <pcap/pcap.h>
#include <atomic>
#include <hound/type/raw_packet_info.hpp>
#include <hound/sink/base_sink.hpp>
#include <condition_variable>
#include <hound/type/deleters.hpp>

namespace hd::type {
class LiveParser {
public:
  LiveParser();
  void startCapture();
  void stopCapture();
  ~LiveParser();

private:
  static void liveHandler(u_char*, const pcap_pkthdr*, const u_char*);
  void consumer_job();

private:
  pcap_handle_t mHandle{nullptr};
  uint32_t mLinkType{};
  std::queue<raw_packet> mPacketQueue;
  std::atomic<bool> keepRunning{true};
  std::shared_ptr<BaseSink> mSink;
  std::condition_variable cv_producer;      // 生产者条件变量
  std::condition_variable cv_consumer;      // 消费者条件变量

  mutable std::mutex mQueueLock;
};
} // entity
#endif//SENDKAFKA
#endif//LIVE_PARSER_HPP
