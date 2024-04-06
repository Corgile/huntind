//
// hound-torch / no_producer_err.hpp
// Created by brian on 2024 Apr 05.
//

#ifndef NO_AVAILABLE_PRODUCER_ERR_HPP
#define NO_AVAILABLE_PRODUCER_ERR_HPP
#include <stdexcept>
#include <string>

// 自定义异常类
class NoProducerErr : public std::runtime_error {
public:
  NoProducerErr(const std::string& message)
      : std::runtime_error(message) {
  }
};

#endif //NO_AVAILABLE_PRODUCER_ERR_HPP
