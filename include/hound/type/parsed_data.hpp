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

#include <hound/common/global.hpp>
#include <hound/type/my_pair.hpp>
#include <hound/type/raw_packet_info.hpp>
#include <hound/type/vlan_header.hpp>

#define ETHERTYPE_IPV4 ETHERTYPE_IP

namespace hd::type {
struct ParsedData final {
  /// \<源_宿> 五元组 /// 用作map key的排序五元组
  std::string m5Tuple, mFlowKey, mTimestamp, mCapLen;;

  my_pair<in_addr_t> mIpPair{};
  my_pair<std::string> mPortPair;

  std::string_view mIP4Head, mTcpHead, mUdpHead, mPayload;
  pcap_pkthdr mPcapHead{};

public:
  bool HasContent{true};

  ParsedData() = delete;

  ~ParsedData() = default;

  [[maybe_unused]]
  ParsedData(raw_packet_info const& data);

private:
  [[nodiscard("do not discard"), maybe_unused]]
  bool processRawBytes(std::string_view _byteArr);

  [[nodiscard("do not discard"), maybe_unused]]
  bool processIPv4Packet(char const* _ip_bytes);

  template<uint8_t AssumedIpProtocol, typename HeaderType>
  void inline func(const uint8_t actual_protocol, ip const* _ipv4, std::string_view& head_buff,
            char const* trans_header, const std::string_view suffix) {
    if (AssumedIpProtocol not_eq actual_protocol) return;
    auto const* _tcp = reinterpret_cast<HeaderType const*>(trans_header);
    std::string const sport = std::to_string(ntohs(_tcp->source));
    std::string const dport = std::to_string(ntohs(_tcp->dest));
    m5Tuple.append(sport).append("_").append(dport).append(suffix);
    this->mPortPair = std::minmax(sport, dport);
    mFlowKey.append(mPortPair.minVal).append("_")
      .append(mPortPair.maxVal).append(suffix);
    size_t tl_hl = 8; // Transport layer header length, 8 for udp
    if (actual_protocol == IPPROTO_TCP) {
      tl_hl = reinterpret_cast<tcphdr const*>(trans_header)->doff * 4;
    }
    // Transport layer head
    head_buff = {trans_header, tl_hl};
    const int available = ntohs(_ipv4->ip_len) - _ipv4->ip_hl * 4 - tl_hl;
    size_t const payload_len = std::min(available, global::opt.payload);
    this->mPayload = {&trans_header[tl_hl], payload_len};
  }
};

} // hd

#endif //HOUND_PARSED_DATA_HPP
