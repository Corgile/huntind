//
// Created by brian on 12/7/23.
//

#ifndef HOUND_FLOW_CHECK_HPP
#define HOUND_FLOW_CHECK_HPP

#include <librdkafka/rdkafkacpp.h>
#include <hound/type/hd_flow.hpp>
#include <hound/common/global.hpp>

#if defined(HD_WITH_KAFKA)
#include <hound/sink/impl/kafka/kafka_config.hpp>
#include <hound/sink/impl/kafka/callback/cb_hash_partitioner.hpp>
#include <hound/sink/impl/kafka/callback/cb_producer_delivery_report.hpp>
#include <hound/sink/impl/kafka/callback/cb_producer_event.hpp>
#endif
namespace flow {
using namespace hd::entity;
using conf_uptr = std::unique_ptr<RdKafka::Conf>;
using c_conf = kafka_config::connection_conf;

#if defined(HD_WITH_KAFKA)
static void InitGetConf(c_conf const& _conn, conf_uptr& _kafkaConf, conf_uptr& _topic) {
  using namespace RdKafka;
  std::string error_buffer;
  // 创建配置对象
  _kafkaConf.reset(Conf::create(Conf::CONF_GLOBAL));
  _kafkaConf->set("bootstrap.servers", _conn.servers, error_buffer);
  _kafkaConf->set("dr_cb", new ProducerDeliveryReportCb, error_buffer);
  // 设置生产者事件回调
  _kafkaConf->set("event_cb", new ProducerEventCb, error_buffer);
  _kafkaConf->set("statistics.interval.ms", "10000", error_buffer);
  //  1MB
  _kafkaConf->set("max.message.bytes", "104858800", error_buffer);
  _kafkaConf->set("enable.manual.events.poll", "false", error_buffer);
  // 1.2、创建 Topic Conf 对象
  _topic.reset(Conf::create(Conf::CONF_TOPIC));
  _topic->set("partitioner_cb", new HashPartitionerCb, error_buffer);
}

static void LoadKafkaConfig(kafka_config& config, std::string const& fileName) {
  std::ifstream config_file(fileName);
  if (not config_file.good()) {
    hd_line(RED("无法打开配置文件: "), fileName);
    exit(EXIT_FAILURE);
  }
  std::string line;
  while (std::getline(config_file, line)) {
    size_t pos{line.find('=')};
    if (pos == std::string::npos or line.at(0) == '#') continue;
    auto value{line.substr(pos + 1)};
    if (value.empty()) continue;
    auto key{line.substr(0, pos)};
    config.put(key, value);
    hd_line(BLUE("加载配置: "), key, "=", value);
  }
}
#endif
}

namespace flow {
using namespace hd::entity;
using namespace hd::global;

static bool IsFlowReady(packet_list const& existing, hd_packet const& _newPacket) {
  if (existing.size() < opt.min_packets) return false;
  return existing.size() == opt.max_packets or
    existing.back().ts_sec - _newPacket.ts_sec >= opt.flowTimeout;
}

template <typename TimeUnit = std::chrono::seconds>
static long timestampNow() {
  auto const now = std::chrono::system_clock::now();
  auto const duration = now.time_since_epoch();
  return std::chrono::duration_cast<TimeUnit>(duration).count();
}
}
#endif //HOUND_FLOW_CHECK_HPP
