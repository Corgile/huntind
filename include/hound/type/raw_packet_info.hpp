//
// Created by brian on 11/28/23.
//

#ifndef HOUND_RAW_PACKET_INFO_HPP
#define HOUND_RAW_PACKET_INFO_HPP

#include <pcap/pcap.h>
#include <string_view>

namespace hd::type {

struct raw_packet_info {
  pcap_pkthdr info_hdr{};
  // std::shared_ptr<char> byte_arr;
  std::string_view byte_arr;

  raw_packet_info(const pcap_pkthdr* pkthdr, const u_char* packet, int32_t len) {
    this->info_hdr = *pkthdr;
    byte_arr = std::string_view(reinterpret_cast<const char*>(packet), len);
  }
};
} // entity

#endif //HOUND_RAW_PACKET_INFO_HPP
