//
// Created by xhl on 23-5-11.
//

#ifndef HOUND_KAFKA_CONNECTION_HPP
#define HOUND_KAFKA_CONNECTION_HPP

#include <librdkafka/rdkafkacpp.h>
#include <string>

#include <hound/common/macro.hpp>
#include <hound/sink/impl/kafka/kafka_config.hpp>


namespace hd::type {
using namespace RdKafka;

class kafka_connection {
private:
  clock_t _aliveTime{};
  std::atomic_int mPartionToFlush{0};            // 分区编号
  int mMaxPartition{0};            // 分区数量
  std::atomic_bool running{true};

  std::unique_ptr<Topic> mTopicPtr{};                  // Topic对象
  std::unique_ptr<Producer> mProducer{};          // Producer对象

public:
  /**
   * @brief message publisher
   */
  kafka_connection(kafka_config::connection_conf const& conn,
                   std::unique_ptr<Conf> const& kafkaConf,
                   std::unique_ptr<Conf> const& topic) {
    std::string errstr;
    this->mMaxPartition = conn.partition;
    auto const _producer{Producer::create(kafkaConf.get(), errstr)};
    auto const _topic{Topic::create(_producer, conn.topic_str, topic.get(), errstr)};
    this->mTopicPtr.reset(_topic);
    this->mProducer.reset(_producer);
    if (this->mMaxPartition > 1) {
      int32_t counter = 0;
      std::thread([&counter, this] {
        while (running) {
          std::this_thread::sleep_for(std::chrono::seconds(10));
          this->mPartionToFlush.store(counter++ % mMaxPartition);
          counter %= mMaxPartition;
        }
      }).detach();
    } else this->mPartionToFlush = 0;
  }

  /**
   * @brief push Message to Kafka
   * @param payload, _key
   */
  void pushMessage(const std::string& payload, const std::string& _key) const {
    ErrorCode const errorCode = mProducer->produce(
      this->mTopicPtr.get(),
      this->mPartionToFlush,
      Producer::RK_MSG_COPY,
      const_cast<char*>(payload.c_str()),
      payload.size(),
      &_key,
      nullptr);
    // mProducer->poll(10'000); // timeout ms.
    if (errorCode == ERR_NO_ERROR) return;
    hd_line(RED("发送失败: "), err2str(errorCode), CYAN(", 长度: "), payload.size());
    // kafka 队列满，等待 5000 ms
    if (errorCode not_eq ERR__QUEUE_FULL) return;
    mProducer->poll(5000);
  }

  ~kafka_connection() {
    while (mProducer->outq_len() > 0) {
      hd_line(YELLOW("Connection "),
              std::this_thread::get_id(),
              RED(" Waiting for queue len: "),
              mProducer->outq_len());
      mProducer->flush(5000);
    }
    running.store(false);
  }

  /// 刷新连接的起始空闲时刻
  /// 在归还回空闲连接队列之前要记录一下连接开始空闲的时刻
  void refreshAliveTime() { _aliveTime = clock(); }

  // 返回连接空闲的时长
  [[nodiscard]] clock_t getAliveTime() const { return clock() - _aliveTime; }
};
} // namespace xhl
#endif // HOUND_KAFKA_CONNECTION_HPP
