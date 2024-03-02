//
// Created by brian on 11/22/23.
//

#ifndef HOUND_KAFKA_HPP
#define HOUND_KAFKA_HPP

#include <hound/sink/base_sink.hpp>
#include <torch/script.h>
#include <hound/sink/impl/kafka/kafka_config.hpp>
#include <hound/sink/impl/kafka/connection_pool.hpp>
#include <hound/sink/impl/flow_check.hpp>

namespace hd::type {
using namespace hd::entity;
using namespace hd::global;

using packet_list = std::vector<hd_packet>;

class KafkaSink final : public BaseSink {
public:
  explicit KafkaSink(std::string const& fileName);

  ~KafkaSink() override;

  void consumeData(ParsedData const& data) override;

private:
  void sendingJob();
  /// \brief 将mFlowTable里面超过timeout但是数量不足的flow删掉
  void cleanerJob();

  inline bool isAttack(const hd_flow& _flow, float threshold);

  void send(const hd_flow& _flow);

  void sendTheRest();

private:
  std::mutex mtxAccessToFlowTable;
  std::unordered_map<std::string, packet_list> mFlowTable;
  std::mutex mtxAccessToQueue;
  std::queue<hd_flow> mSendQueue;
  std::condition_variable cvMsgSender;

  connection_pool* mConnectionPool;
  std::atomic_bool mIsRunning{true};
  torch::jit::Module mModel;
};

} // entity

#endif //HOUND_KAFKA_HPP
