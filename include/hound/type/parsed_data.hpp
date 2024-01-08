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
#include <hound/type/byte_array.hpp>
#include <hound/type/raw_packet_info.hpp>
#include <hound/type/vlan_header.hpp>
#include <hound/type/pcap_header.hpp>

#define ETHERTYPE_IPV4 ETHERTYPE_IP

namespace hd::type {
struct ParsedData final {
  /// \<源_宿> 五元组
  std::string m5Tuple;
  /// 用作map key的排序五元组
  std::string mFlowKey;

  MyValuePair<in_addr_t> mIpPair;
  MyValuePair<std::string> mPortPair;

  std::string mTimestamp, mCapLen;
  ByteArray mIP4Head, mTcpHead, mUdpHead, mPayload;
  PcapHeader mPcapHead;

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
  bool processRawBytes(std::shared_ptr<byte_t> const& _byteArr) {
#if defined(BENCHMARK)
    ++global::packet_index;
#endif // defined(BENCHMARK)
    u_char* pointer = _byteArr.get();
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
      if (_ether_type == ETHERTYPE_IPV6) {
        hd_debug("ETHERTYPE_IPV6");
      } else
      hd_debug("不是 ETHERTYPE_IPV4/6");
      return false;
    }
    return processIPv4Packet(pointer + static_cast<int>(sizeof(ether_header)));
  }

  [[nodiscard("do not discard")]]
  bool processIPv4Packet(byte_t const* ipv4) {
    auto _ipv4RawBytes = const_cast<u_char*>(ipv4);
    ip const* _ipv4 = reinterpret_cast<ip*>(_ipv4RawBytes);
    uint8_t const _ipProtocol{_ipv4->ip_p};
    if (_ipProtocol not_eq static_cast<uint8_t>(IPPROTO_UDP)
      and _ipProtocol not_eq static_cast<uint8_t>(IPPROTO_TCP)) {
      hd_debug(global::packet_index);
#if defined(BENCHMARK)
      ++global::num_consumed_packet;
#endif // defined(BENCHMARK)
      return false;
    }
    this->mIpPair = std::minmax(_ipv4->ip_src.s_addr, _ipv4->ip_dst.s_addr);

    m5Tuple.append(inet_ntoa(_ipv4->ip_src)).append("_")
           .append(inet_ntoa(_ipv4->ip_dst)).append("_");

    mFlowKey.append(inet_ntoa({mIpPair.minVal})).append("_")
            .append(inet_ntoa({mIpPair.maxVal})).append("_");

    int32_t const _ipv4HL = _ipv4->ip_hl * 4;
    this->mIP4Head = {_ipv4RawBytes, _ipv4HL};

    byte_t* _tcpOrUdp{&_ipv4RawBytes[_ipv4HL]};
    // TODO 策略
    if (_ipProtocol == IPPROTO_TCP) {
      tcphdr const* _tcp = reinterpret_cast<tcphdr*>(_tcpOrUdp);
      std::string const sport{std::to_string(ntohs(_tcp->th_sport))};
      std::string const dport{std::to_string(ntohs(_tcp->th_dport))};
      m5Tuple.append(sport).append("_").append(dport).append("_TCP");
      this->mPortPair = std::minmax(sport, dport);
      mFlowKey.append(mPortPair.minVal).append("_")
              .append(mPortPair.maxVal).append("_TCP");
      int const _tcpHL{_tcp->th_off * 4};
      // tcp head
      this->mTcpHead = {_tcpOrUdp, _tcpHL};
      // 处理payload
      int const _byteLen = std::min(ntohs(_ipv4->ip_len) - _ipv4HL - _tcpHL, global::opt.payload);
      this->mPayload = {&_ipv4RawBytes[_ipv4HL + _tcpHL], _byteLen};
    }
    if (_ipProtocol == IPPROTO_UDP) {
      udphdr const* _udp = reinterpret_cast<udphdr*>(_tcpOrUdp);
      std::string const sport{std::to_string(ntohs(_udp->uh_sport))};
      std::string const dport{std::to_string(ntohs(_udp->uh_dport))};
      m5Tuple.append(sport).append("_").append(dport).append("_UDP");
      this->mPortPair = std::minmax(sport, dport);
      mFlowKey.append(mPortPair.minVal).append("_")
              .append(mPortPair.maxVal).append("_UDP");
      int constexpr _udpHL = 8;
      // udp head
      this->mUdpHead = {_tcpOrUdp, _udpHL};
      // 处理payload
      int const _byteLen{std::min(ntohs(_ipv4->ip_len) - _ipv4HL - 8, global::opt.payload)};
      this->mPayload = {&_ipv4RawBytes[_ipv4HL + _udpHL], _byteLen};
    }
    return true;
  }
};
} // hd

#endif //HOUND_PARSED_DATA_HPP
