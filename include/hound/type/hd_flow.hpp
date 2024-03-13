//
// Created by brian on 11/28/23.
//

#ifndef HOUND_HD_FLOW_T_HPP
#define HOUND_HD_FLOW_T_HPP

#include <string>
#include <vector>
#include <pcap/pcap.h>
#include <ylt/struct_json/json_writer.h>

namespace hd::type {

struct hd_packet {
  long ts_sec{};
  long ts_usec{};
  uint32_t packet_len{};
  std::string bitvec;
  hd_packet() = default;
  hd_packet(pcap_pkthdr const& _pcapHead);
};
using packet_list = std::vector<hd_packet>;
REFLECTION(hd_packet, ts_usec, ts_sec, packet_len, bitvec)

struct hd_flow {
  std::string flowId;
  size_t count{};
  packet_list data;
  hd_flow() = default;
  hd_flow(std::string _flowId, packet_list _data);
};

REFLECTION(hd_flow, flowId, count, data)
} // type

#endif //HOUND_HD_FLOW_T_HPP
