//
// Created by brian on 11/22/23.
//

#ifndef HOUND_CAPTURE_OPTION_HPP
#define HOUND_CAPTURE_OPTION_HPP

#include <cstdint>
#include <string>

namespace hd::type {
struct capture_option final {
  int32_t payload{20};
  uint32_t total_bytes{128};
  int32_t num_packets{-1};
  int32_t flowTimeout{20};
  bool include_ts{false};
  bool include_pktlen{false};
  bool include_5tpl{false};
  bool verbose{false};
  int32_t fill_bit{0};

  int32_t stride{8};
  int32_t workers{1};
  int32_t wait1{3};
  int32_t wait2{10};
  std::string device{};
  std::string filter{};
  std::string output_file{};
  std::string model_path{};
  int32_t min_packets{10};
  int32_t max_packets{100};

  int32_t duration{-1};
  std::string separator{','};
  char format[8] = {'%', 'l', 'd', ','};
  size_t poolSize{4};
  uint32_t num_gpus{4};
  uint32_t num_cpus{16};
  std::string brokers{};
  std::string topic{};

  ~capture_option();
};
}

#endif //HOUND_CAPTURE_OPTION_HPP
