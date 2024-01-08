//
// Created by brian on 11/28/23.
//

#ifndef HOUND_STRIDE_T_HPP
#define HOUND_STRIDE_T_HPP

#include <cstdint>

namespace hd::type {

template<int8_t _size>
struct stride_t {
  int64_t buffer: _size;
}__attribute__((__packed__));

} // entity

#endif //HOUND_STRIDE_T_HPP
