//
// hound-torch / producer.hpp
// Created by brian on 2024 Apr 05.
//

#ifndef HD_AFKA_PRODUCER_HPP
#define HD_AFKA_PRODUCER_HPP

#include <memory>
#include <librdkafka/rdkafkacpp.h>
#include <hound/common/macro.hpp>

namespace hd::sink {
class ManagedProducer {
public:
  ManagedProducer(RdKafka::Producer* producer)
    : producer_(producer), stop_polling_(false) {
    polling_thread_ = std::thread([this] {
      while (not stop_polling_) {
        producer_->poll(1000);
      }
    });
  }

  ~ManagedProducer() {
    easylog::set_console(true);
    stop_polling_ = true;
    if (polling_thread_.joinable()) {
      polling_thread_.join();
    }
    if (not producer_) return;
    const auto err_code = producer_->flush(10'000);
    if (err_code not_eq RdKafka::ERR_NO_ERROR) [[unlikely]] {
      ELOG_ERROR << "error code: " << err_code;
    }
    delete producer_;
  }

  RdKafka::Producer* get() const { return producer_; }

  bool isValid() const { return producer_ not_eq nullptr; }

private:
  RdKafka::Producer* producer_;
  std::thread polling_thread_;
  std::atomic<bool> stop_polling_;
};

using ProducerManager = std::unique_ptr<ManagedProducer>;
}

#endif //HD_AFKA_PRODUCER_HPP
