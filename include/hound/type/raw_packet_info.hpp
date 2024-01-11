//
// Created by brian on 11/28/23.
//

#ifndef HOUND_RAW_PACKET_INFO_HPP
#define HOUND_RAW_PACKET_INFO_HPP

#include <pcap/pcap.h>

namespace hd::type {

struct raw_packet {
  // pcap_pkthdr info_hdr;
  using pkhd_t = pcap_pkthdr;
  using byte_t = uint8_t;
  // std::shared_ptr<byte_t const> byte_arr;
  // std::shared_ptr<pkhd_t const> info_hdr;

  byte_t const* byte_arr;
  pkhd_t const* info_hdr;

  raw_packet(pcap_pkthdr const* pkthdr, byte_t const* packet) {
    this->info_hdr = pkthdr;
    this->byte_arr = packet;
  }

  raw_packet(const raw_packet& other) {
    byte_arr = other.byte_arr;
    info_hdr = other.info_hdr;
  }

  raw_packet(raw_packet&& other) noexcept {
    byte_arr = other.byte_arr;
    info_hdr = other.info_hdr;
  }

  raw_packet& operator=(raw_packet other) {
    using std::swap;
    swap(*this, other);
    return *this;
  }
};
} // entity

#endif //HOUND_RAW_PACKET_INFO_HPP
