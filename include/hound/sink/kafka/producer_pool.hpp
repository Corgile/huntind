//
// hound-torch / producer_pool.hpp
// Created by brian on 2024 Apr 05.
//

#ifndef PRODUCERPOOL_HPP
#define PRODUCERPOOL_HPP

#include <hound/sink/kafka/producer.hpp>
#include <hound/sink/kafka/callback.hpp>

struct KafkaConf {
  KafkaConf() = default;
  KafkaConf(const std::string& brokers) {
    conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    partitioner = new MyPartitionCB();
    delivery_cb = new MyReportCB();
    event_cb = new MyEventCB();
    std::string errstr;
    conf->set("bootstrap.servers", brokers, errstr);
    conf->set("partitioner_cb", partitioner, errstr);
    conf->set("dr_cb", delivery_cb, errstr);
    conf->set("event_cb", event_cb, errstr);
  }
  RdKafka::Conf* get() const {
    return conf;
  }
  ~KafkaConf() {
    delete event_cb;
    delete delivery_cb;
    delete partitioner;
    delete conf;
  }

public:
  /*
  KafkaConf(const KafkaConf& other)
    : conf{other.conf},
      partitioner{other.partitioner},
      delivery_cb{other.delivery_cb},
      event_cb{other.event_cb} {}

  KafkaConf& operator=(const KafkaConf& other) {
    if (this == &other) return *this;
    conf = other.conf;
    partitioner = other.partitioner;
    delivery_cb = other.delivery_cb;
    event_cb = other.event_cb;
    return *this;
  }
  */
  KafkaConf(const KafkaConf& other) = delete;
  KafkaConf& operator=(const KafkaConf& other) = delete;

  KafkaConf(KafkaConf&& other) noexcept
    : conf{other.conf},
      partitioner{other.partitioner},
      delivery_cb{other.delivery_cb},
      event_cb{other.event_cb} {
    other.conf = nullptr;
    other.partitioner = nullptr;
    other.delivery_cb = nullptr;
    other.event_cb = nullptr;
  }

  /**
   * @brief move initializer
   * @param other other
   * @return KafkaConf
   */
  KafkaConf& operator=(KafkaConf&& other) noexcept {
    if (this == &other) return *this;
    conf = other.conf;
    partitioner = other.partitioner;
    delivery_cb = other.delivery_cb;
    event_cb = other.event_cb;
    other.conf = nullptr;
    other.partitioner = nullptr;
    other.delivery_cb = nullptr;
    other.event_cb = nullptr;
    return *this;
  }

private:
  RdKafka::Conf* conf;
  MyPartitionCB* partitioner;
  MyReportCB* delivery_cb;
  MyEventCB* event_cb;
};

class ProducerPool {
public:
  ProducerPool() = default;

  ProducerPool(size_t poolSize, const std::string& brokers): kafkaConf_(brokers) {
    std::string errstr;
    producers_.reserve(poolSize);
    for (size_t i = 0; i < poolSize; ++i) {
      const auto producer = RdKafka::Producer::create(kafkaConf_.get(), errstr);
      if (!producer) {
        throw std::runtime_error("Failed to create producer: " + errstr);
      }
      producers_.emplace_back(hd::sink::ProducerUp(producer));
    }
  }

  hd::sink::ProducerUp acquire() {
    std::scoped_lock lock(mutex_);
    if (producers_.empty()) {
      return generate();
    }
    auto producer = std::move(producers_.back());
    producers_.pop_back();
    return producer;
  }

  void collect(hd::sink::ProducerUp producer) {
    std::scoped_lock lock(mutex_);
    producers_.emplace_back(std::move(producer));
  }

public:
  ProducerPool& operator=(ProducerPool&& other) noexcept {
    if (this == &other) return *this;
    producers_ = std::move(other.producers_);
    kafkaConf_ = std::move(other.kafkaConf_);
    return *this;
  }

private:
  hd::sink::ProducerUp generate() {
    std::string errstr;
    producers_.reserve(producers_.size() + 1);
    return hd::sink::ProducerUp(RdKafka::Producer::create(kafkaConf_.get(), errstr));
  }

private:
  KafkaConf kafkaConf_;
  std::vector<hd::sink::ProducerUp> producers_;
  std::mutex mutex_;
};

#endif //PRODUCERPOOL_HPP
