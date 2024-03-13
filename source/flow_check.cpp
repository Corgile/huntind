//
// Created by brian on 3/13/24.
//
#include <hound/common/flow_check.hpp>

bool flow::_isTimeout(hd::type::packet_list const& existing, hd::type::hd_packet const& new_) {
  if (existing.empty()) return false;
  return new_.ts_sec - existing.back().ts_sec >= opt.flowTimeout;
}

bool flow::_isTimeout(hd::type::packet_list const& existing) {
  long const now = flow::timestampNow<std::chrono::seconds>();
  return now - existing.back().ts_sec >= opt.flowTimeout;
}

bool flow::_isLengthSatisfied(hd::type::packet_list const& existing) {
  return existing.size() >= opt.min_packets and existing.size() <= opt.max_packets;
}

bool flow::IsFlowReady(hd::type::packet_list const& existing, hd::type::hd_packet const& new_) {
  if (existing.size() == opt.max_packets) return true;
  return _isTimeout(existing, new_) and _isLengthSatisfied(existing);
}

void flow::InitGetConf(hd::type::kafka_config const& conn_conf, RdConfUptr& _serverConf, RdConfUptr& _topic) {
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
