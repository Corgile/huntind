//
// Created by brian on 11/28/23.
//

#ifndef HOUND_HD_FLOW_HPP
#define HOUND_HD_FLOW_HPP

#include <string>
#include <vector>
#include <pcap/pcap.h>
#include <ostream>
#include <hound/type/parsed_packet.hpp>
#include <ylt/struct_json/json_writer.h>

namespace hd::type {
/*struct hd_packet {
  long ts_sec{};
  long ts_usec{};
  uint32_t actual_len{};
  std::string_view raw;

  hd_packet() = default;

  hd_packet(pcap_pkthdr const& _pcap_head);

  friend std::ostream& operator<<(std::ostream& os, hd_packet const& packet) {
    os << "{data: [..." << packet.raw.length() << " bytes...]"
      << ",actual_len: " << packet.actual_len
      << ",mTsSec: " << packet.ts_sec
      << ",mTSuSec: ";
    if (packet.ts_usec < 100000) os << "0";
    os << packet.ts_usec << "}";
    return os;
  }
};*/

template <typename T>
concept OutStream = requires(T& os, const std::string& s)
{
  { os << s } -> std::same_as<T&>;
};

struct hd_flow {
  uint16_t protocol{};
  size_t count{};
  std::string flowId;
  packet_list _packet_list;

  hd_flow() = default;

  hd_flow(std::string _flowId, packet_list& _data);

  template <OutStream stream>
  void print(stream& out) const {
    out << "Flow ID: " << flowId
      << "\tProtocol: " << (int)protocol
      << "\tCount: " << count << "\n";
    int i = 0;
    for (auto const& item : _packet_list) {
      out << "\t" << item;
      if (i++ == 5) break;
    }
    out << "\n";
  }

  friend std::ostream& operator<<(std::ostream& out, hd_flow const& _flow) {
    out << "Flow ID: " << _flow.flowId
      << "\tProtocol: " << (int)_flow.protocol
      << "\tCount: " << _flow.count << "\n";
    int i = 0;
    for (auto const& item : _flow._packet_list) {
      out << "\t" << item << "\n";
      if (i++ == 5) break;
    }
    return out;
  }
};

REFLECTION(hd_flow, protocol, count, flowId, _packet_list)

} // type

#endif //HOUND_HD_FLOW_HPP
