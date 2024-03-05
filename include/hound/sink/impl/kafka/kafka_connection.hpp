//
// Created by xhl on 23-5-11.
//

#ifndef HOUND_KAFKA_CONNECTION_HPP
#define HOUND_KAFKA_CONNECTION_HPP

#include <librdkafka/rdkafkacpp.h>
#include <string>

#include <hound/common/macro.hpp>
#include <hound/sink/impl/kafka/kafka_config.hpp>
//#include <hound/sink/compress.hpp>
// #include <my-timer.hpp>


namespace hd::entity {
using namespace RdKafka;

class kafka_connection {
private:
  clock_t _aliveTime{};
  std::atomic_int mPartionToFlush{0};            // 分区编号
  int mMaxPartition{0};            // 分区数量
  std::atomic_bool running{true};
  Topic* mTopicPtr;
  Producer* mProducer;

public:
  /**
   * @brief message publisher
   */
  kafka_connection(kafka_config::_conn const& conn,
                   std::unique_ptr<Conf> const& kafkaConf,
                   std::unique_ptr<Conf> const& topic) {
    std::string errstr;
    this->mMaxPartition = conn.partition;
    // TODO: Producer::create 疑似存在内存泄漏
    this->mProducer = Producer::create(kafkaConf.get(), errstr);
    this->mTopicPtr = Topic::create(mProducer, conn.topic_str, topic.get(), errstr);
    if (this->mMaxPartition > 1) {
      int32_t counter = 0;
      std::thread([&counter, this] {
        using namespace std::chrono_literals;
        while (running) {
          std::this_thread::sleep_for(20s);
          this->mPartionToFlush.store(counter++ % mMaxPartition);
          counter %= mMaxPartition;
        }
      }).detach();
    } else this->mPartionToFlush = 0;
  }

  /**
   * @brief push Message to Kafka
   * @param payload: 就是 payload
   * @param _key: 就是 flowId
   */
  void pushMessage(const std::string_view payload, const std::string& _key) const {
    ErrorCode const errorCode = mProducer->produce(
      this->mTopicPtr, this->mPartionToFlush,
      Producer::RK_MSG_COPY, (void*) payload.data(),
      payload.size(), &_key, nullptr
    );
    if (errorCode == ERR_NO_ERROR) return;
    hd_line(RED("发送失败: "), err2str(errorCode), CYAN(", 长度: "), payload.size());
    if (errorCode not_eq ERR__QUEUE_FULL) return;
    // kafka 队列满，等待 5000 ms
    mProducer->poll(5'000);
  }

  ~kafka_connection() {
    running.store(false);
    while (mProducer->outq_len() > 0) {
      hd_line(YELLOW("kafka连接["), std::this_thread::get_id(),
              YELLOW("]的缓冲队列: "), mProducer->outq_len());
      mProducer->flush(5'000);
    }
    /// 有先后之分，先topic 再producer
    delete mTopicPtr;
    delete mProducer;
  }

  /// 刷新连接的起始空闲时刻 <br>
  /// 在归还回空闲连接队列之前要记录一下连接开始空闲的时刻
  void refreshAliveTime() { _aliveTime = clock(); }

  // 返回连接空闲的时长
  [[nodiscard]] clock_t getAliveTime() const { return clock() - _aliveTime; }
};
} // namespace xhl
#endif // HOUND_KAFKA_CONNECTION_HPP
