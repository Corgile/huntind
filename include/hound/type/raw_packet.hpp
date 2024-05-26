//
// Created by brian on 11/28/23.
//

#ifndef HOUND_RAW_PACKET_INFO_HPP
#define HOUND_RAW_PACKET_INFO_HPP

#include <pcap/pcap.h>
#include <string_view>
#include <vector>
#include <ylt/util/concurrentqueue.h>

namespace hd::type {

struct raw_packet {
  raw_packet() = default;
  pcap_pkthdr info_hdr{};
  std::string_view byte_arr;

  raw_packet(const pcap_pkthdr*, const u_char*, uint32_t);
};
// using raw_vector = std::vector<raw_packet>;
using raw_vector = moodycamel::ConcurrentQueue<raw_packet>;
} // type

#endif //HOUND_RAW_PACKET_INFO_HPP
