//
// Created by brian on 11/28/23.
//

#ifndef HOUND_RAW_PACKET_INFO_HPP
#define HOUND_RAW_PACKET_INFO_HPP

#include <pcap/pcap.h>
#include <cstring>

namespace hd::type {
using byte_t = uint8_t;

struct raw_packet_info {
  pcap_pkthdr info_hdr;
  std::shared_ptr<byte_t> byte_arr;

  raw_packet_info(const pcap_pkthdr* pkthdr, const byte_t* packet) {
    this->info_hdr = *pkthdr;//copy
    // this->byte_arr.reset(packet);
    // eth, vlan, ip,
    this->byte_arr.reset(new byte_t[pkthdr->caplen + 1]);
    std::memcpy(this->byte_arr.get(), packet, pkthdr->caplen);
  }
};
} // entity

#endif //HOUND_RAW_PACKET_INFO_HPP
