//
// hound-torch / producer_pool.hpp
// Created by brian on 2024 Apr 05.
//

#ifndef PRODUCERPOOL_HPP
#define PRODUCERPOOL_HPP

#include <hound/scope_guard.hpp>
#include <hound/sink/kafka/producer.hpp>

class ProducerPool {
public:
  ProducerPool() = default;

  ProducerPool(size_t poolSize, const std::string& brokers) {
    std::string errstr;
    const auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    scope_guard<void> _guard([&conf] {
      delete conf;
    });
    conf->set("bootstrap.servers", brokers, errstr);
    producers_.reserve(poolSize);
    for (size_t i = 0; i < poolSize; ++i) {
      auto producer = RdKafka::Producer::create(conf, errstr);
      if (!producer) {
        throw std::runtime_error("Failed to create producer: " + errstr);
      }
      producers_.emplace_back(hd::sink::ProducerUp(producer));
    }
  }

  hd::sink::ProducerUp acquire() {
    std::scoped_lock lock(mutex_);
    if (producers_.empty()) {
      throw NoProducerErr("producers queue empty: 无可用连接");
    }
    auto producer = std::move(producers_.back());
    producers_.pop_back();
    return producer;
  }

  hd::sink::ProducerUp generate() {
    std::string errstr;
    const auto conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    scope_guard<void> _guard([&conf] {
      delete conf;
    });
    producers_.reserve(producers_.size() + 1);
    return std::move(hd::sink::ProducerUp(RdKafka::Producer::create(conf, errstr)));
  }

  void collect(hd::sink::ProducerUp producer) {
    std::scoped_lock lock(mutex_);
    producers_.emplace_back(std::move(producer));
  }

public:
  ProducerPool& operator=(ProducerPool&& other) noexcept {
    if (this == &other) return *this;
    producers_.swap(other.producers_);
    return *this;
  }

private:
  std::vector<hd::sink::ProducerUp> producers_;
  std::mutex mutex_;
};

#endif //PRODUCERPOOL_HPP
