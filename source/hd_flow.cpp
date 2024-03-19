//
// Created by brian on 3/13/24.
//
#include "hound/type/hd_flow.hpp"

hd::type::hd_packet::hd_packet(pcap_pkthdr const& _pcap_head) {
  ts_sec = _pcap_head.ts.tv_sec;
  ts_usec = _pcap_head.ts.tv_usec;
  actual_len = _pcap_head.caplen;
}

hd::type::hd_flow::hd_flow(std::string _flowId, packet_list _data) {
  flowId = std::move(_flowId);
  _packet_list = std::move(_data);
  count = _packet_list.size();
}
