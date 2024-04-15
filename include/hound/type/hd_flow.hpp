//
// Created by brian on 11/28/23.
//

#ifndef HOUND_HD_FLOW_HPP
#define HOUND_HD_FLOW_HPP

#include <string>
//#include <vector>
#include <ylt/util/concurrentqueue.h>
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
  long count{};
  std::string flowId;
  parsed_vector _packet_list;

  parsed_packet& at(size_t idx);

  hd_flow() = default;

  hd_flow(std::string _flowId, parsed_vector& _data);

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

using flow_queue = moodycamel::ConcurrentQueue<hd_flow>;
using flow_vector = std::vector<hd_flow>;
using flow_vec_ref = std::vector<std::reference_wrapper<hd_flow>>;
} // type

#endif //HOUND_HD_FLOW_HPP
