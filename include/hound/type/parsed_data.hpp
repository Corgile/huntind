//
// Created by brian on 11/22/23.
//

#ifndef HOUND_PARSED_DATA_HPP
#define HOUND_PARSED_DATA_HPP

#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <hound/type/raw_packet_info.hpp>
#include <hound/type/vlan_header.hpp>

#define ETHERTYPE_IPV4 ETHERTYPE_IP

namespace hd::type {
struct ParsedData final {
  using byte_t = uint8_t;
  std::string m5Tuple;
  std::int32_t version{4};
  std::uint32_t sPort{0}, dPort{0}, uSec{0}, Sec{0}, capLen{0}, pktCode{0};
  std::uint32_t sIP{}, dIP{};

  bool HasContent{true};

  ParsedData() = delete;

  ~ParsedData() = default;

  ParsedData(raw_packet const& data) {
    uSec = data.info_hdr->ts.tv_usec;
    Sec = data.info_hdr->ts.tv_sec;
    capLen = data.info_hdr->caplen;
    this->HasContent = processRawBytes(data.byte_arr);
  }

private:
  [[nodiscard("do not discard")]]
  // bool processRawBytes(std::shared_ptr<byte_t const> const& _byteArr) {
  bool processRawBytes(byte_t const* _byteArr) {
    byte_t const* pointer = _byteArr;
    auto eth{reinterpret_cast<ether_header const*>(pointer)};
    if (ntohs(eth->ether_type) == ETHERTYPE_VLAN) {
      pointer += static_cast<int>(sizeof(vlan_header));
      eth = reinterpret_cast<ether_header const*>(pointer);
    }
    const auto _ether_type{ntohs(eth->ether_type)};
    if (_ether_type not_eq ETHERTYPE_IPV4) return false;
    constexpr int offset{sizeof(ether_header)};
    return processIPv4Packet(pointer + offset);
  }

  [[nodiscard("do not discard")]]
  bool processIPv4Packet(byte_t const* ip4RawBytes) {
    const auto _ipv4{reinterpret_cast<ip const*>(ip4RawBytes)};
    uint8_t const _ipProtocol{_ipv4->ip_p};
    if (_ipProtocol not_eq IPPROTO_UDP and _ipProtocol not_eq IPPROTO_TCP) {
      hd_debug(global::packet_index);
      return false;
    }
    this->sIP = _ipv4->ip_src.s_addr;
    this->dIP = _ipv4->ip_dst.s_addr;
    m5Tuple.append(inet_ntoa(_ipv4->ip_src)).append("-")
           .append(inet_ntoa(_ipv4->ip_dst)).append("-");
    int32_t const _ipv4HL = _ipv4->ip_hl * 4;
    byte_t const* _tcpOrUdp{&ip4RawBytes[_ipv4HL]};
    func<IPPROTO_TCP, tcphdr>(_ipProtocol, _tcpOrUdp);
    func<IPPROTO_UDP, udphdr>(_ipProtocol, _tcpOrUdp);
    return true;
  }

  template <uint8_t AssumedIpProtocol, typename HeaderType>
  void func(const uint8_t actual_ip_protocal, byte_t const* _tcpOrUdp) {
    if (actual_ip_protocal not_eq AssumedIpProtocol) return;
    auto tcpOrUdp = reinterpret_cast<HeaderType const*>(_tcpOrUdp);
    this->sPort = ntohs(tcpOrUdp->source);
    this->dPort = ntohs(tcpOrUdp->dest);
    auto const sport{std::to_string(this->sPort)};
    auto const dport{std::to_string(this->dPort)};
    m5Tuple.append(sport).append("-")
           .append(dport).append("-")
           .append(std::to_string(actual_ip_protocal));
    pktCode = IPPROTO_UDP;
    if (actual_ip_protocal == IPPROTO_TCP) {
      pktCode = reinterpret_cast<tcphdr const*>(_tcpOrUdp)->th_flags;
    }
  }
};
} // hd

#endif //HOUND_PARSED_DATA_HPP
