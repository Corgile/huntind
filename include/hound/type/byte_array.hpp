//
// Created by brian on 11/28/23.
//

#ifndef HOUND_BYTE_ARRAY_HPP
#define HOUND_BYTE_ARRAY_HPP

#include <cstdint>
#include <string>

namespace hd::type {
using byte_t = uint8_t;

struct ByteArray {
  byte_t* data;
  int32_t byteLen = 0;

  ByteArray() = default;

  /// list initializer
  ByteArray(byte_t* data, int32_t const byte_len)
      : data(data),
        byteLen(byte_len) {
  }

  template<typename stride_t>
  std::string dump() {
    if (byteLen <= 0) return {};
    std::string ret;
    auto arr = reinterpret_cast<stride_t*>(data);
    for (int i = 0; i < byteLen - 1; ++i) {
      auto v = reinterpret_cast<decltype(stride_t::buffer)>(arr[i]);
      ret.append(std::to_string(v)).append(global::opt.separator);
    }
    auto v = reinterpret_cast<decltype(stride_t::buffer)>(arr[byteLen - 1]);
    ret.append(std::to_string(v));
    return ret;
  }
};


} // entity

#endif //HOUND_BYTE_ARRAY_HPP
