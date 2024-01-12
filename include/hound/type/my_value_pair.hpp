//
// Created by brian on 11/28/23.
//

#ifndef HOUND_MY_VALUE_PAIR_HPP
#define HOUND_MY_VALUE_PAIR_HPP

#include <algorithm>

namespace hd::type {

template<typename T>
struct MyValuePair {
  T minVal, maxVal;

  MyValuePair() = default;

  MyValuePair(T& _v1, T& _v2)
      : minVal{std::min(_v1, _v2)},
        maxVal{std::max(_v1, _v2)} {
    // std::tie(std::minmax({v1, v2}));
  }

  template<typename U, typename V>
  MyValuePair(const std::pair<U, V>& pair)
      : minVal{pair.first}, maxVal{pair.second} {
  }
};

} // entity

#endif //HOUND_MY_VALUE_PAIR_HPP
