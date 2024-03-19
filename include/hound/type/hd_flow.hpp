//
// Created by brian on 11/28/23.
//

#ifndef HOUND_HD_FLOW_HPP
#define HOUND_HD_FLOW_HPP

#include <string>
#include <vector>
#include <pcap/pcap.h>
#include <ylt/struct_json/json_writer.h>

namespace hd::type {

struct hd_packet {
  long ts_sec{};
  long ts_usec{};
  uint32_t actual_len{};
  std::string_view raw;

  hd_packet() = default;

  hd_packet(pcap_pkthdr const& _pcap_head);
};

using packet_list = std::vector<hd_packet>;
REFLECTION(hd_packet, ts_usec, ts_sec, actual_len, raw)

struct hd_flow {
  uint8_t protocol{};
  size_t count{};
  std::string flowId;
  packet_list _packet_list;

  hd_flow() = default;

  hd_flow(std::string _flowId, packet_list _data);
};

REFLECTION(hd_flow, protocol, count, flowId, _packet_list)
} // type

#endif //HOUND_HD_FLOW_HPP
