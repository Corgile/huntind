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
using namespace hd::entity;
using namespace hd::global;
using packet_list = std::vector<hd_packet>;

template<typename TimeUnit = std::chrono::seconds>
static long timestampNow() {
  auto const now = std::chrono::system_clock::now();
  auto const duration = now.time_since_epoch();
  return std::chrono::duration_cast<TimeUnit>(duration).count();
}

inline bool _isTimeout(packet_list const& existing, hd_packet const& new_) {
  if (existing.empty()) return false;
  return new_.ts_sec - existing.back().ts_sec >= opt.flowTimeout;
}

inline bool _isTimeout(packet_list const& existing) {
  long const now = flow::timestampNow<std::chrono::seconds>();
  return now - existing.back().ts_sec >= opt.flowTimeout;
}

inline bool _isLengthSatisfied(packet_list const& existing) {
  return existing.size() >= opt.min_packets and existing.size() <= opt.max_packets;
}

static bool IsFlowReady(packet_list const& existing, hd_packet const& new_) {
  if (existing.size() == opt.max_packets) return true;
  return _isTimeout(existing, new_) and _isLengthSatisfied(existing);
}

static void InitGetConf(kafka_config const& conn_conf,
                        std::unique_ptr<RdKafka::Conf>& _serverConf,
                        std::unique_ptr<RdKafka::Conf>& _topic) {
  using namespace RdKafka;
  std::string error_buffer;
  // 创建配置对象
  _serverConf.reset(Conf::create(Conf::CONF_GLOBAL));
  _serverConf->set("bootstrap.servers", conn_conf.servers, error_buffer);
  _serverConf->set("dr_cb", new ProducerDeliveryReportCb, error_buffer);
  // 设置生产者事件回调
  _serverConf->set("event_cb", new ProducerEventCb, error_buffer);
  _serverConf->set("statistics.interval.ms", "10000", error_buffer);
  //  1MB
  _serverConf->set("max.message.bytes", "104858800", error_buffer);
  // 1.2、创建 Topic Conf 对象
  _topic.reset(Conf::create(Conf::CONF_TOPIC));
  _topic->set("partitioner_cb", new HashPartitionerCb, error_buffer);
}
}
#endif //HOUND_FLOW_CHECK_HPP
