//
// Created by brian on 11/28/23.
//

#ifndef HOUND_BYTE_ARRAY_HPP
#define HOUND_BYTE_ARRAY_HPP

#include <cstdint>

namespace hd::type {
using byte_t = uint8_t;

struct ByteArray {
  uint64_t* data;
  int32_t byteLen = 0;

  ByteArray() = default;

  /// list initializer
  ByteArray(byte_t* data, int32_t const byte_len)
      : data(reinterpret_cast<uint64_t*>(data)),
        byteLen(byte_len) {
  }
};


} // entity

#endif //HOUND_BYTE_ARRAY_HPP
