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
  struct _conn {
    std::string servers;
    std::string topic_str;

    int32_t partition{0};
    int32_t max_idle{60};
    int32_t timeout_sec{2};
  } conn;
  /// 连接池配置
  struct _pool {
    int32_t init_size{3};
    int32_t max_size{5};
  } pool;

  void put(const std::string& k, const std::string& v) {
    if (k == hd::keys::KAFKA_BROKERS)
      this->conn.servers = v;
    if (k == hd::keys::KAFKA_TOPICS)
      this->conn.topic_str = v;

    if (k == hd::keys::KAFKA_PARTITION)
      this->conn.partition = std::stoi(v);
    if (k == hd::keys::CONN_MAX_IDLE_S)
      this->conn.max_idle = std::stoi(v);
    if (k == hd::keys::CONN_TIMEOUT_MS)
      this->conn.timeout_sec = std::stoi(v);

    if (k == hd::keys::POOL_INIT_SIZE)
      this->pool.init_size = std::stoi(v);
    if (k == hd::keys::POOL_MAX_SIZE)
      this->pool.max_size = std::stoi(v);
  }
};

} // namespace xhl

#endif //HOUND_KAFKA_CONFIG_HPP
