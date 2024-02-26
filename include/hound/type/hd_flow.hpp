//
// Created by brian on 11/28/23.
//

#ifndef HOUND_HD_FLOW_T_HPP
#define HOUND_HD_FLOW_T_HPP

#include <string>
#include <vector>
#include <pcap/bpf.h>
#include <hound/type/pcap_header.hpp>
#include <struct_json/json_writer.h>

namespace hd::entity {
struct hd_packet {
  __time_t ts_sec{};
  __suseconds_t ts_usec{};
  bpf_u_int32 packet_len{};
  std::string bitvec;

  hd_packet() = default;

  explicit hd_packet(const type::PcapHeader& _pcapHead) {
    ts_sec = _pcapHead.ts_sec;
    ts_usec = _pcapHead.ts_usec;
    packet_len = _pcapHead.caplen;
  }

  hd_packet(const hd_packet& other)
    : ts_sec(other.ts_sec),
      ts_usec(other.ts_usec),
      packet_len(other.packet_len),
      bitvec(other.bitvec) {
  }

  hd_packet(hd_packet&& other) noexcept
    : ts_sec(other.ts_sec),
      ts_usec(other.ts_usec),
      packet_len(other.packet_len),
      bitvec(std::move(other.bitvec)) {
  }

  hd_packet& operator=(const hd_packet& other) {
    if (this == &other) return *this;
    ts_sec = other.ts_sec;
    ts_usec = other.ts_usec;
    packet_len = other.packet_len;
    bitvec = other.bitvec;
    return *this;
  }

  hd_packet& operator=(hd_packet&& other) noexcept {
    if (this == &other) return *this;
    ts_sec = other.ts_sec;
    ts_usec = other.ts_usec;
    packet_len = other.packet_len;
    bitvec = std::move(other.bitvec);
    return *this;
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
#ifdef HD_DEV
  size_t size() const {
    auto s = flowId.size() + sizeof(struct hd_flow);
    for (auto _packet : data) {
      s += sizeof(hd_packet) + _packet.bitvec.size();
    }
    return s / 1024;
  }
#endif


  hd_flow() = default;
};

REFLECTION(hd_flow, flowId, count, data)
} // entity

#endif //HOUND_HD_FLOW_T_HPP
