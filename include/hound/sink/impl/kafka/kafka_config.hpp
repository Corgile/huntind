//
// Created by gzhuadmin on 7/25/23.
//

#ifndef HOUND_KAFKA_CONFIG_HPP
#define HOUND_KAFKA_CONFIG_HPP

#include <cstdint>
#include <string>

#include <hound/sink/impl/kafka/constants.hpp>

namespace hd::entity {
struct kafka_config {
  /// 连接参数
  struct connection_conf {
    std::string servers;
    std::string topic_str;
    int32_t partition{0};
    int32_t max_idle{60};
    int32_t timeout_sec{20};
  } conn;

  /// 连接池配置
  struct _pool {
    int32_t init_size{3};
    int32_t max_size{5};
  } pool;

  void put(const std::string& k, const std::string& v) {
    //@formatter:off
    if (k == keys::KAFKA_BROKERS)   this->conn.servers.assign(v);
    if (k == keys::KAFKA_TOPICS)    this->conn.topic_str.assign(v);
    if (k == keys::KAFKA_PARTITION) this->conn.partition = std::stoi(v);
    if (k == keys::CONN_MAX_IDLE_S) this->conn.max_idle = std::stoi(v);
    if (k == keys::CONN_TIMEOUT_MS) this->conn.timeout_sec = std::stoi(v);
    if (k == keys::POOL_INIT_SIZE)  this->pool.init_size = std::stoi(v);
    if (k == keys::POOL_MAX_SIZE)   this->pool.max_size = std::stoi(v);
  }//@formatter:on
};
} // namespace xhl

#endif // HOUND_KAFKA_CONFIG_HPP
