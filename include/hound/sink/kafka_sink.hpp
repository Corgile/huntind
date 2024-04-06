//
// Created by brian on 11/22/23.
//

#ifndef HOUND_KAFKA_HPP
#define HOUND_KAFKA_HPP

#include <torch/torch.h>

#include <hound/type/hd_flow.hpp>
#include <hound/type/parsed_packet.hpp>
#include <ylt/util/concurrentqueue.h>

namespace hd::sink {
using namespace hd::type;
using namespace hd::global;
using namespace std::chrono_literals;

using RdConfUptr [[maybe_unused]] = std::unique_ptr<RdKafka::Conf>;
using flow_map = std::unordered_map<std::string, parsed_list>;
using flow_iter = flow_map::iterator;

class KafkaSink final {
public:
  KafkaSink();

  void MakeFlow(std::shared_ptr<raw_list> const &_raw_list);

  ~KafkaSink();

private:
  void sendToKafkaTask();

  static torch::Tensor EncodeFlowList(const flow_vector &_flow_list,
                                      torch::Tensor const &slide_window);

  /// \brief 将<code>mFlowTable</code>里面超过 timeout 但是数量不足的flow删掉
  void cleanUnwantedFlowTask();

  int SendEncoding(std::shared_ptr<flow_vector> const &long_flow_list);

  static void SplitFlows(std::shared_ptr<flow_vector> const &_list,
                         std::vector<flow_vector> &output, const size_t &by);

  static void _EncodeAndSend(flow_vector &_flow_list);

private:
  std::mutex mtxAccessToFlowTable;
  flow_map mFlowTable;

  std::condition_variable cvMsgSender;

  flow_queue mSendQueue;

  std::thread mSendTask;
  std::thread mCleanTask;

  std::atomic_bool mIsRunning{true};
};
} // namespace hd::sink

#endif // HOUND_KAFKA_HPP
