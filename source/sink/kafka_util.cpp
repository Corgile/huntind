//
// Created by brian on 3/13/24.
//
#include <hound/sink/kafka/kafka_util.hpp>

#include <hound/sink/kafka/callback/cb_producer_delivery_report.hpp>
#include <hound/sink/kafka/callback/cb_hash_partitioner.hpp>
#include <hound/sink/kafka/callback/cb_producer_event.hpp>

void hd::util::InitGetConf(kafka_config const& conn_conf,
                           RdConfUptr& _serverConf,
                           RdConfUptr& _topic) {
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
