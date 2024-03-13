//
// Created by brian on 11/28/23.
//

#ifndef HOUND_MY_PAIR_HPP
#define HOUND_MY_PAIR_HPP

#include <algorithm>

namespace hd::type {

template<typename T>
struct my_pair {
  T minVal, maxVal;

  my_pair() = default;

  [[maybe_unused]] my_pair(T& _v1, T& _v2) {
    minVal = std::min(_v1, _v2);
    maxVal = std::max(_v1, _v2);
  }

  template<typename U, typename V>
  [[maybe_unused]] my_pair(const std::pair<U, V>& pair) {
    minVal = pair.first;
    maxVal = pair.second;
  }
};

} // type

#endif //HOUND_MY_PAIR_HPP
