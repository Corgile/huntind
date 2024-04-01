//
// Created by brian on 11/22/23.
//

#ifndef HOUND_KAFKA_HPP
#define HOUND_KAFKA_HPP

#include <hound/common/scope_guard.hpp>
#include <hound/sink/connection_pool.hpp>
#include <hound/sink/kafka/kafka_connection.hpp>
#include <hound/type/hd_flow.hpp>
#include <hound/type/parsed_packet.hpp>

#include <ylt/util/concurrentqueue.h>

namespace hd::sink {
using namespace hd::type;
using namespace hd::global;
using namespace std::chrono_literals;

using RdConfUptr [[maybe_unused]] = std::unique_ptr<Conf>;
using flow_map = std::unordered_map<std::string, parsed_list>;
using flow_iter = flow_map::iterator;

class KafkaSink final {
public:
  KafkaSink();

  void MakeFlow(std::shared_ptr<raw_list> const &_raw_list);

  ~KafkaSink();

private:
  void sendToKafkaTask();

  torch::Tensor EncodFlowList(const flow_vector &_flow_list,
                              torch::Tensor const &slide_window);

  /// \brief 将<code>mFlowTable</code>里面超过 timeout 但是数量不足的flow删掉
  void cleanUnwantedFlowTask();

  int SendEncoding(std::shared_ptr<flow_vector> const &long_flow_list);

  static void SplitFlows(std::shared_ptr<flow_vector> const &_list,
                         std::vector<flow_vector> &output, const size_t &by);

  void _EncodeAndSend(flow_vector &_flow_list);

private:
  std::mutex mtxAccessToFlowTable;
  flow_map mFlowTable;

  std::condition_variable cvMsgSender;

  //  std::mutex mtxAccessToQueue;
  flow_queue mSendQueue;
  //  moodycamel::ConcurrentQueue<hd_flow> mSendQueue;

  std::thread mSendTask;
  std::thread mCleanTask;

  ModelPool mPool;

  struct Creator {
    kafka_connection *operator()() const {
      return new kafka_connection(global::KafkaConfig);
    }

    kafka_connection *operator()(const bool inUse) const {
      const auto conn = new kafka_connection(global::KafkaConfig);
      conn->setInUse(inUse);
      return conn;
    }
  };

  ConnectionPool<kafka_connection, Creator> mConnectionPool;

  std::atomic_bool mIsRunning{true};
};
} // namespace hd::sink

#endif // HOUND_KAFKA_HPP
