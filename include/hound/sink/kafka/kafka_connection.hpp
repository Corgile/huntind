//
// Created by xhl on 23-5-11.
//

#ifndef HOUND_KAFKA_CONNECTION_HPP
#define HOUND_KAFKA_CONNECTION_HPP

#include <string>
#include <librdkafka/rdkafkacpp.h>

#include <hound/common/macro.hpp>
#include <hound/sink/kafka/kafka_config.hpp>

namespace hd::type {
using namespace RdKafka;
using RdConfUptr = std::unique_ptr<RdKafka::Conf>;

class kafka_connection {
private:
  clock_t _idleStart{};
  std::atomic_int mPartitionToFlush{0};            // 分区编号
  int mMaxPartition{0};            // 分区数量
  int mMaxIdle{};
  std::atomic_bool mInUse{};
  std::atomic_bool mIsAlive{true};
  Topic* mTopicPtr{};
  Producer* mProducer{};

public:
  /**
   * @brief message publisher
   */
  kafka_connection(kafka_config const& conn);

  /**
   * @brief push Message to Kafka
   * @param payload: 就是 payload
   * @param payload_size: 就是 payload 大小
   * @param ordered_key: 有序flowId（与encoding顺序一致）
   */
  [[nodiscard]]
  int pushMessage(void* payload, size_t payload_size, std::string const& ordered_key) const;

  ~kafka_connection();

  /// 刷新连接的起始空闲时刻 <br>
  /// 在归还回空闲连接队列之前要记录一下连接开始空闲的时刻
  [[maybe_unused]]
  void resetIdleTime();

  [[maybe_unused]] [[nodiscard]]
  bool isRedundant() const;

  /// 设置`正在使用`标识
  [[maybe_unused]]
  void setInUse(bool v);

  /// 获取使用状态
  [[nodiscard]]
  bool isInUse() const;

private:
  /// 返回连接空闲的时长
  [[nodiscard]]
  clock_t getIdleTime() const;
};
} // namespace xhl
#endif // HOUND_KAFKA_CONNECTION_HPP
