//
// Created by brian on 3/13/24.
//
#include <cstring>
#include <ylt/easylog.hpp>
#include <netinet/udp.h>
#include "hound/type/parsed_packet.hpp"
#include "hound/type/vlan_header.hpp"

[[maybe_unused]]
hd::type::parsed_packet::parsed_packet(raw_packet const& raw) {
  this->mCapLen = raw.info_hdr.caplen;
  this->mTsSec = raw.info_hdr.ts.tv_sec;
  this->mTSuSec = raw.info_hdr.ts.tv_usec;
  this->HasContent = processRawBytes(raw.byte_arr);
}

bool hd::type::parsed_packet::processRawBytes(std::string_view _byteArr) {
  char const* pointer = _byteArr.data();
  auto eth{reinterpret_cast<ether_header const*>(pointer)};
  if (ntohs(eth->ether_type) == ETHERTYPE_VLAN) {
    pointer += static_cast<int>(sizeof(vlan_header));
    eth = reinterpret_cast<ether_header const*>(pointer);
  }
  auto const _ether_type = ntohs(eth->ether_type);
  if (_ether_type not_eq ETHERTYPE_IPV4) {
    return false;
  }
  return processIPv4Packet(pointer + static_cast<int>(sizeof(ether_header)));
}

bool hd::type::parsed_packet::processIPv4Packet(char const* _ip_bytes) {
  auto const _ipv4 = reinterpret_cast<ip const*>(_ip_bytes);
  uint8_t const _protocol{_ipv4->ip_p};
  if (_protocol != IPPROTO_UDP and _protocol != IPPROTO_TCP) {
    return false;
  }
  auto my_minmax = [](ip const* ip_head)-> auto {
    return ip_head->ip_src.s_addr > ip_head->ip_dst.s_addr
             ? std::make_pair(ip_head->ip_dst, ip_head->ip_src)
             : std::make_pair(ip_head->ip_src, ip_head->ip_dst);
  };
  auto [ip1, ip2] = my_minmax(_ipv4);
  mKey.append(inet_ntoa(ip1)).append("_").append(inet_ntoa(ip2)).append("_");
  size_t const _ipv4HL = _ipv4->ip_hl * 4;
  mBlobData.append(_ip_bytes, _ipv4HL);
  if (_ipv4HL < 60) [[likely]] {//ip header是变长的，需要补0
    mBlobData.append(nullptr, 60 - _ipv4HL);
  }
  if (_protocol == IPPROTO_TCP) {
    mBlobData.append(&_ip_bytes[_ipv4HL], 60);
    mBlobData.append(nullptr, 8);
    CopyPayloadToBlob<tcphdr>(_protocol, _ipv4, &_ip_bytes[_ipv4HL], "_TCP");
  } else if (_protocol == IPPROTO_UDP) {
    mBlobData.append(&_ip_bytes[_ipv4HL], 8);
    mBlobData.append(nullptr, 60);
    CopyPayloadToBlob<udphdr>(_protocol, _ipv4, &_ip_bytes[_ipv4HL], "_UDP");
  }
  return true;
}

/// @brief c'tor
/// @param copied other instance
hd::type::parsed_packet::parsed_packet(const parsed_packet& copied) {
  mTsSec = copied.mTsSec;
  mTSuSec = copied.mTSuSec;
  mCapLen = copied.mCapLen;
  mKey = copied.mKey;
  this->mBlobData.assign(copied.mBlobData);
  HasContent = copied.HasContent;
}

hd::type::parsed_packet::parsed_packet(parsed_packet&& other) noexcept {
  mTsSec = other.mTsSec;
  mTSuSec = other.mTSuSec;
  mCapLen = other.mCapLen;
  mKey = std::move(other.mKey);
  mBlobData = std::move(other.mBlobData);
  HasContent = other.HasContent;
}

hd::type::parsed_packet& hd::type::parsed_packet::operator=(parsed_packet other) {
  using std::swap;
  swap(*this, other);
  return *this;
}
