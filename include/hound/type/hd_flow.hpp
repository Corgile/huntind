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

namespace hd::type {

template <typename T>
concept OutStream = requires(T& os, const std::string& s)
{
  { os << s } -> std::same_as<T&>;
};

struct hd_flow {
  uint16_t protocol{};
  size_t count{};
  std::string flowId;
  parsed_list _packet_list;

  hd_flow() = default;

  hd_flow(std::string _flowId, parsed_list& _data);

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

using flow_list = std::vector<hd_flow>;

} // type

#endif //HOUND_HD_FLOW_HPP
