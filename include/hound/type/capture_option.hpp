//
// Created by brian on 11/22/23.
//

#ifndef HOUND_CAPTURE_OPTION_HPP
#define HOUND_CAPTURE_OPTION_HPP

#include <cstdint>
#include <string>

namespace hd::type {
struct capture_option final {
  capture_option() = default;
  std::int32_t worker{1};
  std::string filter{};
  std::string csv;
  std::string pcap;
  std::string out;
  std::int32_t num{-1};
};
}

#endif //HOUND_CAPTURE_OPTION_HPP
