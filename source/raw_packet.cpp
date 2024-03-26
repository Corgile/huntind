//
// Created by brian on 3/13/24.
//
#include "hound/type/raw_packet.hpp"

hd::type::raw_packet::raw_packet(pcap_pkthdr const* pkthdr, u_char const* packet, int32_t len)
  : info_hdr(*pkthdr), byte_arr(reinterpret_cast<const char*>(packet), len) {}
