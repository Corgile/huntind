//
// Created by brian on 3/5/24.
//

#ifndef HOUND_KAFKA_CONFIG_HPP
#define HOUND_KAFKA_CONFIG_HPP

#include <cstdint>
#include <string>
#include <fstream>
#include <hound/common/macro.hpp>
#include <hound/sink/impl/kafka/constants.hpp>

namespace hd::type {

struct kafka_config {
  /// 连接参数
  std::string servers;
  std::string topic_str;

  int32_t partition{0};
  int32_t max_idle{60};

  void read_kafka_conf(std::string const& fileName);

  void put(const std::string& k, const std::string& v);
};

} // namespace xhl

#endif //HOUND_KAFKA_CONFIG_HPP
