//
// Created by xhl on 23-5-11.
//

#ifndef HOUND_KAFKA_CONNECTION_HPP
#define HOUND_KAFKA_CONNECTION_HPP

#include <librdkafka/rdkafkacpp.h>
#include <string>

#include <hound/common/macro.hpp>
#include <hound/sink/impl/kafka/kafka_config.hpp>

namespace hd::entity {
using namespace RdKafka;

class kafka_connection {
private:
  clock_t _idleStart{};
  std::atomic_int mPartitionToFlush{0};            // 分区编号
  int mMaxPartition{0};            // 分区数量
  int mMaxIdle;
  std::atomic_bool mInUse;
  std::atomic_bool mIsAlive{true};
  Topic* mTopicPtr;
  Producer* mProducer;

public:
  /**
   * @brief message publisher
   */
  kafka_connection(kafka_config const& conn,
                   std::unique_ptr<RdKafka::Conf>const & producer_conf,
                   std::unique_ptr<RdKafka::Conf>const & _topic) {
    std::string errstr;
    this->mMaxPartition = conn.partition;
    this->mMaxIdle = conn.max_idle;
    this->mInUse = false;
    this->mProducer = Producer::create(producer_conf.get(), errstr);
    this->mTopicPtr = Topic::create(mProducer, conn.topic_str, _topic.get(), errstr);
    if (this->mMaxPartition > 1) {
      int32_t counter = 0;
      std::thread([&counter, this] {
        using namespace std::chrono_literals;
        while (mIsAlive) {
          std::this_thread::sleep_for(20s);
          this->mPartitionToFlush.store(counter++ % mMaxPartition);
          counter %= mMaxPartition;
        }
      }).detach();
    } else this->mPartitionToFlush = 0;
  }

  /**
   * @brief push Message to Kafka
   * @param payload: 就是 payload
   * @param _key: 就是 flowId
   */
  [[nodiscard]] int pushMessage(const std::string_view payload, const std::string& _key) const {
    ErrorCode const errorCode = mProducer->produce(
      this->mTopicPtr, this->mPartitionToFlush,
      Producer::RK_MSG_COPY, (void*) payload.data(),
      payload.size(), &_key, nullptr
    );
    if (errorCode == ERR_NO_ERROR) return 0;
    ELOG_ERROR << RED("发送失败: ") << err2str(errorCode) << CYAN(", 长度: ") << payload.size();
    if (errorCode not_eq ERR__QUEUE_FULL) return 1;
    // kafka 队列满，等待 5000 ms
    mProducer->poll(5'000);
    return 1;
  }

  ~kafka_connection() {
    mIsAlive.store(false);
    while (mProducer->outq_len() > 0) {
      mProducer->flush(5'000);
    }
    /// 有先后之分，先topic 再producer
    ELOG_DEBUG << "kafka连接 ["
               << std::this_thread::get_id()
               << "] 的缓冲队列: "
               << mProducer->outq_len();
    delete mTopicPtr;
    delete mProducer;
  }

  /// 刷新连接的起始空闲时刻 <br>
  /// 在归还回空闲连接队列之前要记录一下连接开始空闲的时刻
  void resetIdleTime() { _idleStart = clock(); }

  [[nodiscard]] bool isRedundant() const {
    return getIdleTime() >= mMaxIdle * 1000 and not isInUse();
  }

  /// 设置`正在使用`标识
  void setInUse(bool v) {
    this->mInUse = v;
  }

private:

  /// 返回连接空闲的时长
  [[nodiscard]] clock_t getIdleTime() const { return clock() - _idleStart; }

  /// 获取使用状态
  [[nodiscard]] bool inline isInUse() const {
    return mInUse;
  }
};
} // namespace xhl
#endif // HOUND_KAFKA_CONNECTION_HPP
