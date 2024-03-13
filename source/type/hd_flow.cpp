//
// Created by brian on 3/13/24.
//
#include <hound/type/hd_flow.hpp>

hd::type::hd_packet::hd_packet(pcap_pkthdr const& _pcapHead) {
  ts_sec = _pcapHead.ts.tv_sec;
  ts_usec = _pcapHead.ts.tv_usec;
  packet_len = _pcapHead.caplen;
}

hd::type::hd_flow::hd_flow(std::string _flowId, packet_list _data) {
  flowId = std::move(_flowId);
  data = std::move(_data);
  count = data.size();
}
