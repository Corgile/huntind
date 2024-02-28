//
// Created by brian on 11/28/23.
//

#ifndef HOUND_RAW_PACKET_INFO_HPP
#define HOUND_RAW_PACKET_INFO_HPP

#include <pcap/pcap.h>

namespace hd::type {
using byte_t = char;

struct raw_packet_info {
  pcap_pkthdr info_hdr{};
  // std::shared_ptr<char> byte_arr;
  std::string_view byte_arr;

  raw_packet_info(const pcap_pkthdr* pkthdr, const u_char* packet, int32_t len) {
    this->info_hdr = *pkthdr;
    // auto v = std::hash<std::shared_ptr<char>>{}(byte_arr);
    // this->byte_arr.reset(new char[len + 1], [&](char *p) {
    //   std::cout << "释放内存: "<< v << "\n";
    //   delete[] p;
    // });
    // std::cout << "开辟内存: "<< v << "\n";
    // std::memcpy(this->byte_arr.get(), packet, len);
    // this->byte_arr.get()[len] = '\0';
    byte_arr = std::string_view(reinterpret_cast<const char*>(packet), len);
  }
};
} // entity

#endif //HOUND_RAW_PACKET_INFO_HPP
