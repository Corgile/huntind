//
// Created by brian on 11/22/23.
//

#ifndef HOUND_CAPTURE_OPTION_HPP
#define HOUND_CAPTURE_OPTION_HPP

#include <cstdint>
#include <string>

namespace hd::type {
struct capture_option final {
  explicit capture_option() = default;

  /// proto filter
  bool include_ip4{true};
  bool include_tcp{true};
  bool include_udp{true};
  /// config
  int32_t payload{20};
  int32_t num_packets{-1};
  int32_t packetTimeout{20};
  bool verbose{false};
  bool include_ts{false};
  bool include_pktlen{false};
  bool include_5tpl{false};
  int32_t fill_bit{0};

  int32_t stride{8};
  int32_t workers{1};
  std::string device{};
  std::string filter{"(tcp or udp)"};
  std::string output_file{};
  int32_t min_packets{10};
  int32_t max_packets{100};

  /// mode
  bool write_file{false};
  std::string pcap_file{};
  char format[8] = {'%', 'u', ','};
  std::string_view separator{","};

public:
  void print() const;

  ~capture_option();
};
}

#endif //HOUND_CAPTURE_OPTION_HPP
