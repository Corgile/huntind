//
// Created by brian on 11/22/23.
//

#ifndef HOUND_KAFKA_HPP
#define HOUND_KAFKA_HPP

#ifdef LATENCY_TEST
  #include <fstream>
#endif

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
using flow_iter = std::unordered_map<std::string, packet_list>::iterator;

class KafkaSink final {
public:
  KafkaSink(const kafka_config&, const RdConfUptr&, const RdConfUptr&);

  void consume_data(raw_packet const& raw);

  ~KafkaSink();

private:
  void sendToKafkaTask();

  /// \brief 将<code>mFlowTable</code>里面超过 timeout 但是数量不足的flow删掉
  void cleanUnwantedFlowTask();

  // TODO: 改为发送流的encoding
  int send(const hd_flow& flow) const;

private:
  std::mutex mtxAccessToFlowTable;
  std::unordered_map<std::string, packet_list> mFlowTable;

  std::condition_variable cvMsgSender;

  std::mutex mtxAccessToQueue;
  std::queue<hd_flow> mSendQueue;

  std::thread mSendTask;
  std::thread mCleanTask;

  kafka_connection* pConnection;
  std::atomic_bool mIsRunning{true};
#ifdef LATENCY_TEST
  std::ofstream mTimestampLog;
  std::mutex mFileAccess;
#endif
};
} // type

#endif //HOUND_KAFKA_HPP
