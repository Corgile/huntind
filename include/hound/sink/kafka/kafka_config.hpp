//
// Created by brian on 3/5/24.
//

#ifndef HOUND_KAFKA_CONFIG_HPP
#define HOUND_KAFKA_CONFIG_HPP

#include <cstdint>
#include <string>
#include <librdkafka/rdkafkacpp.h>

#include "hound/common/macro.hpp"

namespace hd::type {
using RdConfUptr = std::unique_ptr<RdKafka::Conf>;

struct kafka_config {
  kafka_config() = default;

  kafka_config(std::string const& fileName) {
    read_kafka_conf(fileName);
    init_configuration();
  }

  /// 连接参数
  std::string servers;
  std::string topic_str;

  int32_t partition{0};
  int32_t max_idle{60};

  RdConfUptr mServerConf;
  RdConfUptr mTopicConf;

  void read_kafka_conf(std::string const& fileName);

  void put(const std::string& k, const std::string& v);

  void init_configuration();
};
} // namespace xhl

#endif //HOUND_KAFKA_CONFIG_HPP
