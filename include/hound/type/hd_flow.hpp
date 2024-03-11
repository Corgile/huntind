//
// Created by brian on 11/28/23.
//

#ifndef HOUND_HD_FLOW_T_HPP
#define HOUND_HD_FLOW_T_HPP

#include <string>
#include <vector>
#include <pcap/pcap.h>
#include <ylt/struct_json/json_writer.h>

namespace hd::entity {
struct hd_packet {
  __time_t ts_sec{};
  __suseconds_t ts_usec{};
  bpf_u_int32 packet_len{};
  std::string bitvec;

  hd_packet() = default;

  explicit hd_packet(pcap_pkthdr const& _pcapHead) {
    ts_sec = _pcapHead.ts.tv_sec;
    ts_usec = _pcapHead.ts.tv_usec;
    packet_len = _pcapHead.caplen;
  }
};

REFLECTION(hd_packet, ts_usec, ts_sec, packet_len, bitvec)

struct hd_flow {
  std::string flowId;
  size_t count{};
  std::vector<hd_packet> data;

  hd_flow(std::string flowId, std::vector<hd_packet> _data)
    : flowId(std::move(flowId)),
      data(std::move(_data)) {
    count = data.size();
  }

  hd_flow() = default;
};

REFLECTION(hd_flow, flowId, count, data)
} // entity

#endif //HOUND_HD_FLOW_T_HPP
