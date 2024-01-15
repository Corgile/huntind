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
  int32_t flowTimeout{20};
  bool verbose{false};
  bool unsign{false};
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
#if defined(HD_OFFLINE)
  std::string pcap_file{};
#endif
  char format[3] = {'%', 'd', ','};
  std::string separator {","};
#if defined(HD_WITH_KAFKA)
  std::string kafka_config;
  int32_t duration{-1};
#endif

public:
  void print() const;

  ~capture_option();
};
}

#endif //HOUND_CAPTURE_OPTION_HPP
