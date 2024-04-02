//
// Created by brian on 3/13/24.
//
#include "hound/sink/kafka/kafka_config.hpp"
#include "hound/sink/kafka/constants.hpp"
#include "hound/sink/kafka/callback/cb_producer_delivery_report.hpp"
#include "hound/sink/kafka/callback/cb_hash_partitioner.hpp"
#include "hound/sink/kafka/callback/cb_producer_event.hpp"
#include "hound/common/macro.hpp"

void hd::type::kafka_config::read_kafka_conf(std::string const& fileName) {
  std::ifstream config_file(fileName);
  if (not config_file.good()) {
    ELOG_ERROR << RED("无法打开配置文件: ") << fileName;
    exit(EXIT_FAILURE);
  }
  std::string line;
  while (std::getline(config_file, line)) {
    size_t pos{line.find('=')};
    if (pos == std::string::npos or line.at(0) == '#') continue;
    auto value{line.substr(pos + 1)};
    if (value.empty()) continue;
    auto key{line.substr(0, pos)};
    this->put(key, value);
    ELOG_WARN << BLUE("加载配置: ") << key << "=" << value;
  }
}

void hd::type::kafka_config::put(std::string const& k, std::string const& v) {
  if (k == hd::keys::KAFKA_BROKERS) this->servers = v;
  if (k == hd::keys::KAFKA_TOPICS) this->topic_str = v;

  if (k == hd::keys::KAFKA_PARTITION) this->partition = std::stoi(v);
  if (k == hd::keys::CONN_MAX_IDLE_S) this->max_idle = std::stoi(v);
}

void hd::type::kafka_config::init_configuration() {
  std::string error_buffer;
  using namespace RdKafka;
  // 创建配置对象
  mServerConf.reset(Conf::create(Conf::CONF_GLOBAL));
  mServerConf->set("bootstrap.servers", this->servers, error_buffer);
  mServerConf->set("dr_cb", new ProducerDeliveryReportCb, error_buffer);
  // 设置生产者事件回调
  mServerConf->set("event_cb", new ProducerEventCb, error_buffer);
  mServerConf->set("statistics.interval.ms", "10000", error_buffer);
  //  1MB
  mServerConf->set("max.message.bytes", "104858800", error_buffer);
  // 1.2、创建 Topic Conf 对象
  mTopicConf.reset(Conf::create(Conf::CONF_TOPIC));
  mTopicConf->set("partitioner_cb", new HashPartitionerCb, error_buffer);
}
