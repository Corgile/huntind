//
// Created by brian on 3/13/24.
//
#include "hound/type/raw_packet_info.hpp"

hd::type::raw_packet_info::raw_packet_info(pcap_pkthdr const* pkthdr, u_char const* packet, int32_t len) {
  this->info_hdr = *pkthdr;
  byte_arr = std::string_view(reinterpret_cast<const char*>(packet), len);
}