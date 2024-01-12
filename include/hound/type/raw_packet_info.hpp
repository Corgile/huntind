//
// Created by brian on 11/28/23.
//

#ifndef HOUND_RAW_PACKET_INFO_HPP
#define HOUND_RAW_PACKET_INFO_HPP

#include <cstring>
#include <pcap/pcap.h>
#include <hound/common/macro.hpp>
#include <hound/type/vlan_header.hpp>

struct pcap_pkthdr;

namespace hd::type {
using byte_t = uint8_t;


struct raw_packet {
  struct deleter {
    void operator()(byte_t* pointer) const {
      delete[] pointer;
    }
  };

  pcap_pkthdr info_hdr{};
  std::unique_ptr<byte_t, deleter> byte_arr;

  raw_packet(const pcap_pkthdr* pkthdr, const byte_t* packet) {
    using global::opt;
    this->info_hdr = *pkthdr;
    constexpr int hdr_size{sizeof(ether_header) + sizeof(vlan_header) + IP4_PADSIZE + TCP_PADSIZE + UDP_PADSIZE};
    const auto fixed_len{hdr_size + opt.payload};
    this->byte_arr.reset(new byte_t[fixed_len]);
    std::memcpy(this->byte_arr.get(), packet, fixed_len);
  }
};
} // entity

#endif //HOUND_RAW_PACKET_INFO_HPP
