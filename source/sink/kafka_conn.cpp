//
// Created by brian on 3/13/24.
//
#include <hound/sink/impl/kafka/kafka_connection.hpp>

hd::type::kafka_connection::kafka_connection(kafka_config const& conn, RdConfUptr const& producer_conf,
                                             RdConfUptr const& _topic) {
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
        std::this_thread::sleep_for(10s);
        this->mPartitionToFlush.store(counter++ % mMaxPartition);
        counter %= mMaxPartition;
      }
    }).detach();
  } else this->mPartitionToFlush = 0;

}

int hd::type::kafka_connection::pushMessage(std::string_view const payload, std::string const& _key) const {
  ErrorCode const errorCode = mProducer->produce(
    this->mTopicPtr, this->mPartitionToFlush,
    Producer::RK_MSG_COPY, (void*) payload.data(),
    payload.size(), &_key, nullptr
  );
  if (errorCode == ERR_NO_ERROR) return 0;
  ELOG_ERROR << RED("发送失败: ") << err2str(errorCode) << CYAN(", 长度: ") << payload.size();
  if (errorCode not_eq ERR__QUEUE_FULL) return 1;
  mProducer->poll(5'000);
  return 1;
}

hd::type::kafka_connection::~kafka_connection() {
  mIsAlive.store(false);
  while (mProducer->outq_len() > 0) {
    mProducer->flush(5'000);
  }
  /// 有先后之分，先topic 再producer
  ELOG_INFO << YELLOW("kafka连接 [")
            << std::this_thread::get_id()
            << YELLOW("] 的缓冲队列: ")
            << mProducer->outq_len();
  delete mTopicPtr;
  delete mProducer;
}

[[maybe_unused]]
void hd::type::kafka_connection::resetIdleTime() {
  _idleStart = clock();
}

[[maybe_unused]]
bool hd::type::kafka_connection::isRedundant() const {
  return getIdleTime() >= mMaxIdle * 1000 and not isInUse();
}

[[maybe_unused]]
void hd::type::kafka_connection::setInUse(bool v) {
  this->mInUse = v;
}

clock_t hd::type::kafka_connection::getIdleTime() const {
  return clock() - _idleStart;
}

bool inline hd::type::kafka_connection::isInUse() const {
  return mInUse;
}
