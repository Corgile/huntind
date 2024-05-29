//
// Created by brian on 11/22/23.
//

#ifndef FC_REFACTORED_LIVE_PARSER_HPP
#define FC_REFACTORED_LIVE_PARSER_HPP

#include <pcap/pcap.h>
#include <atomic>
#include <condition_variable>
#include <hound/type/raw_packet.hpp>
#include <hound/type/deleters.hpp>
#include <hound/sink/kafka_sink.hpp>
#include <hound/task_executor.hpp>

namespace hd::type {
class LiveParser {
public:
  LiveParser();

  void startCapture();

  void stopCapture();

  ~LiveParser();

  bool isRunning() const;

private:
  static void liveHandler(u_char*, const pcap_pkthdr*, const u_char*);

  void consumer_job();

private:
  std::atomic<bool> is_running{true};
  pcap_handle_t mHandle{nullptr};
  std::vector<std::thread> mConsumerTasks;
  TaskExecutor mTaskExecutor;
  DoubleBufferQueue<raw_packet> doubleBufferQueue;
};
} // type

#endif //FC_REFACTORED_LIVE_PARSER_HPP
