//
// Created by brian on 3/13/24.
//
#include "hound/sink/kafka/kafka_config.hpp"

void hd::type::kafka_config::read_kafka_conf(std::string const& fileName) {
  std::ifstream config_file(fileName);
  if (not config_file.good()) {
    hd_println(RED("无法打开配置文件: "), fileName);
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
  if (k == hd::keys::KAFKA_BROKERS)
    this->servers = v;
  if (k == hd::keys::KAFKA_TOPICS)
    this->topic_str = v;

  if (k == hd::keys::KAFKA_PARTITION)
    this->partition = std::stoi(v);
  if (k == hd::keys::CONN_MAX_IDLE_S)
    this->max_idle = std::stoi(v);
}
