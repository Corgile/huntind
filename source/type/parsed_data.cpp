//
// Created by brian on 3/13/24.
//
#include <hound/type/parsed_data.hpp>

[[maybe_unused]]
hd::type::ParsedData::ParsedData(hd::type::raw_packet_info const& data) {
  this->mPcapHead = {
    data.info_hdr.ts.tv_sec,
    data.info_hdr.ts.tv_usec,
    data.info_hdr.caplen
  };
  this->mCapLen.assign(std::to_string(mPcapHead.caplen));
  this->mTimestamp.assign(
    std::to_string(mPcapHead.ts.tv_sec)
      .append(global::opt.separator)
      .append(std::to_string(mPcapHead.ts.tv_usec))
  );
  this->HasContent = processRawBytes(data.byte_arr);
}

bool hd::type::ParsedData::processRawBytes(std::string_view _byteArr) {
#if defined(BENCHMARK)
  ++global::packet_index;
#endif // defined(BENCHMARK)
  char const* pointer = _byteArr.data();//.get();
  auto eth{reinterpret_cast<ether_header const*>(pointer)};
  if (ntohs(eth->ether_type) == ETHERTYPE_VLAN) {
    pointer += static_cast<int>(sizeof(vlan_header));
    eth = reinterpret_cast<ether_header const*>(pointer);
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

bool hd::type::ParsedData::processIPv4Packet(char const* _ip_bytes) {
  auto _ipv4 = reinterpret_cast<ip const*>(_ip_bytes);
  uint8_t const _ipProtocol{_ipv4->ip_p};
  if (_ipProtocol not_eq IPPROTO_UDP and _ipProtocol not_eq IPPROTO_TCP) {
#if defined(BENCHMARK)
    hd_debug("index: ", global::packet_index);
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
