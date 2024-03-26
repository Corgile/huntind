//
// Created by brian on 11/22/23.
//

#ifndef HOUND_KAFKA_HPP
#define HOUND_KAFKA_HPP

#ifdef LATENCY_TEST
  #include <fstream>
#endif

#include <torch/script.h>

#include <hound/sink/kafka/kafka_config.hpp>
#include <hound/sink/kafka/kafka_connection.hpp>
#include <hound/common/core.hpp>
#include <hound/type/parsed_packet.hpp>
#include <hound/type/hd_flow.hpp>

namespace hd::sink {
using namespace hd::type;
using namespace hd::global;
using namespace std::chrono_literals;

using RdConfUptr = std::unique_ptr<Conf>;
using flow_map = std::unordered_map<std::string, parsed_list>;
using flow_iter = flow_map::iterator;

class KafkaSink final {
public:
  KafkaSink(const kafka_config&, const RdConfUptr&, const RdConfUptr&);

  void consume_data(const std::shared_ptr<raw_list>& raw_list);

  int send(std::shared_ptr<flow_list> const& long_flow_list);

  ~KafkaSink();

private:
  void sendToKafkaTask();

  /// \brief 将<code>mFlowTable</code>里面超过 timeout 但是数量不足的flow删掉
  void cleanUnwantedFlowTask();



private:
  std::mutex mtxAccessToFlowTable;
  flow_map mFlowTable;

  std::condition_variable cvMsgSender;

  std::mutex mtxAccessToQueue;
  flow_list mSendQueue;

  std::thread mSendTask;
  std::thread mCleanTask;

  torch::jit::script::Module mModel;

  kafka_connection* pConnection;
  std::atomic_bool mIsRunning{true};
#ifdef LATENCY_TEST
  std::ofstream mTimestampLog;
  std::mutex mFileAccess;
#endif
};
} // type

#endif //HOUND_KAFKA_HPP
