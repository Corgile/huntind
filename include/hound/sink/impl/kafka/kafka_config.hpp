//
// Created by brian on 3/5/24.
//

#ifndef HOUND_KAFKA_CONFIG_HPP
#define HOUND_KAFKA_CONFIG_HPP

#include <cstdint>
#include <string>
#include <hound/sink/impl/kafka/constants.hpp>

namespace hd::entity {

struct kafka_config {
  /// 连接参数
  std::string servers;
  std::string topic_str;

  int32_t partition{0};
  int32_t max_idle{60};
  int32_t timeout_sec{2};

  void read_kafka_conf(std::string const& fileName) {
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

  void put(const std::string& k, const std::string& v) {
    if (k == hd::keys::KAFKA_BROKERS)
      this->servers = v;
    if (k == hd::keys::KAFKA_TOPICS)
      this->topic_str = v;

    if (k == hd::keys::KAFKA_PARTITION)
      this->partition = std::stoi(v);
    if (k == hd::keys::CONN_MAX_IDLE_S)
      this->max_idle = std::stoi(v);
    if (k == hd::keys::CONN_TIMEOUT_MS)
      this->timeout_sec = std::stoi(v);
  }
};

} // namespace xhl

#endif //HOUND_KAFKA_CONFIG_HPP
