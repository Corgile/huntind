//
// hound-torch / producer.hpp
// Created by brian on 2024 Apr 05.
//

#ifndef HD_AFKA_PRODUCER_HPP
#define HD_AFKA_PRODUCER_HPP

#include <memory>
#include <librdkafka/rdkafkacpp.h>
#include <hound/common/macro.hpp>
#include <hound/no_producer_err.hpp>

namespace RdKafka {
struct Deleter {
  void operator()(Producer* ptr) const {
    if (not ptr) return;
    auto err_code = ptr->flush(10'000);
    if (err_code not_eq ERR_NO_ERROR) [[unlikely]] {
      ELOG_ERROR << "error code: " << err_code;
    }
    delete ptr;
  }
};
}//RdKafka

namespace hd::sink {
using ProducerUp = std::unique_ptr<RdKafka::Producer, RdKafka::Deleter>;
}


#endif //HD_AFKA_PRODUCER_HPP
