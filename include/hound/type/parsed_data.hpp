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
#include <hound/type/my_value_pair.hpp>
#include <hound/type/raw_packet_info.hpp>
#include <hound/type/vlan_header.hpp>
#include <hound/type/pcap_header.hpp>

#define ETHERTYPE_IPV4 ETHERTYPE_IP

namespace hd::type {
struct ParsedData final {
  /// \<源_宿> 五元组 /// 用作map key的排序五元组
  std::string m5Tuple, mFlowKey, mTimestamp, mCapLen;;

  MyValuePair<in_addr_t> mIpPair{};
  MyValuePair<std::string> mPortPair;

  std::string_view mIP4Head, mTcpHead, mUdpHead, mPayload;
  PcapHeader mPcapHead{};

public:
  bool HasContent{true};

  ParsedData() = delete;

  ~ParsedData() = default;

  ParsedData(raw_packet_info const& data) {
    this->mPcapHead = {
    data.info_hdr.ts.tv_sec,
    data.info_hdr.ts.tv_usec,
    data.info_hdr.caplen
    };
    this->mCapLen.assign(std::to_string(mPcapHead.caplen));
    this->mTimestamp.assign(
      std::to_string(mPcapHead.ts_sec)
      .append(global::opt.separator)
      .append(std::to_string(mPcapHead.ts_usec))
    );
    this->HasContent = processRawBytes(data.byte_arr);
  }

private:
  [[nodiscard("do not discard")]]
  bool processRawBytes(std::shared_ptr<char> const& _byteArr) {
#if defined(BENCHMARK)
    ++global::packet_index;
#endif // defined(BENCHMARK)
    char* pointer = _byteArr.get();
    auto eth{reinterpret_cast<ether_header*>(pointer)};
    if (ntohs(eth->ether_type) == ETHERTYPE_VLAN) {
      pointer += static_cast<int>(sizeof(vlan_header));
      eth = reinterpret_cast<ether_header*>(pointer);
    }
    auto const _ether_type = ntohs(eth->ether_type);
    if (_ether_type not_eq ETHERTYPE_IPV4) {
#if defined(BENCHMARK)
      ++global::num_consumed_packet;
#endif // defined(BENCHMARK)
#if defined(HD_DEV)
      if (_ether_type == ETHERTYPE_IPV6) {
        hd_debug("不支持: ", "ETHERTYPE_IPV6");
      } else
      hd_debug("不支持: ", "不是 ETHERTYPE_IP");
#endif
      return false;
    }
    return processIPv4Packet(pointer + static_cast<int>(sizeof(ether_header)));
  }

  [[nodiscard("do not discard")]]
  bool processIPv4Packet(char const* _ip_bytes) {
    auto _ipv4 = reinterpret_cast<ip const*>(_ip_bytes);
    uint8_t const _ipProtocol{_ipv4->ip_p};
    if (_ipProtocol not_eq IPPROTO_UDP and _ipProtocol not_eq IPPROTO_TCP) {
#if defined(BENCHMARK)
      hd_debug(global::packet_index);
      ++global::num_consumed_packet;
#endif // defined(BENCHMARK)
      return false;
    }
    this->mIpPair = std::minmax(_ipv4->ip_src.s_addr, _ipv4->ip_dst.s_addr);

    m5Tuple.append(inet_ntoa(_ipv4->ip_src)).append("_")
           .append(inet_ntoa(_ipv4->ip_dst)).append("_");

    mFlowKey.append(inet_ntoa({mIpPair.minVal})).append("_")
            .append(inet_ntoa({mIpPair.maxVal})).append("_");

    size_t const _ipv4HL = _ipv4->ip_hl * 4;
    this->mIP4Head = {_ip_bytes, _ipv4HL};

    func<IPPROTO_TCP, tcphdr>(_ipProtocol, _ipv4, mTcpHead, &_ip_bytes[_ipv4HL], "_TCP");
    func<IPPROTO_UDP, udphdr>(_ipProtocol, _ipv4, mUdpHead, &_ip_bytes[_ipv4HL], "_UDP");
    return true;
  }

  template <uint8_t AssumedIpProtocol, typename HeaderType>
  void func(const uint8_t actual_protocol, ip const* _ipv4, std::string_view& head_buff,
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
