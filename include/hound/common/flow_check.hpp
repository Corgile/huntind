//
// Created by brian on 12/7/23.
//

#ifndef HOUND_FLOW_CHECK_HPP
#define HOUND_FLOW_CHECK_HPP

#include <hound/sink/impl/kafka/kafka_config.hpp>
#include <hound/sink/impl/kafka/callback/cb_producer_delivery_report.hpp>
#include <hound/sink/impl/kafka/callback/cb_hash_partitioner.hpp>
#include <hound/sink/impl/kafka/callback/cb_producer_event.hpp>
#include <hound/type/hd_flow.hpp>
#include <hound/common/global.hpp>
#include <librdkafka/rdkafkacpp.h>

namespace flow {
using namespace hd::type;
using namespace hd::global;
using namespace RdKafka;
using RdConfUptr = std::unique_ptr<RdKafka::Conf>;

template<typename TimeUnit = std::chrono::seconds>
static long timestampNow() {
  auto const now = std::chrono::system_clock::now();
  auto const duration = now.time_since_epoch();
  return std::chrono::duration_cast<TimeUnit>(duration).count();
}

inline bool _isTimeout(packet_list const& existing, hd_packet const& new_);

inline bool _isTimeout(packet_list const& existing);

inline bool _isLengthSatisfied(packet_list const& existing);

inline bool IsFlowReady(packet_list const& existing, hd_packet const& new_);

inline void InitGetConf(kafka_config const& conn_conf, RdConfUptr& _serverConf, RdConfUptr& _topic);
}
#endif //HOUND_FLOW_CHECK_HPP
