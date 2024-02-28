//
// Created by brian on 12/7/23.
//

#ifndef HOUND_FLOW_CHECK_HPP
#define HOUND_FLOW_CHECK_HPP

#include <hound/type/hd_flow.hpp>
#include <hound/common/global.hpp>

namespace flow {
using namespace hd::entity;
using namespace hd::global;
using PacketList = std::vector<hd_packet>;

static bool IsFlowReady(PacketList const& existing, hd_packet const& newPacket) {
  if (existing.size() < opt.min_packets) return false;
  return existing.size() == opt.max_packets or
    existing.back().ts_sec - newPacket.ts_sec >= opt.flowTimeout;
}

template <typename TimeUnit = std::chrono::seconds>
static long timestampNow() {
  auto const now = std::chrono::system_clock::now();
  auto const duration = now.time_since_epoch();
  return std::chrono::duration_cast<TimeUnit>(duration).count();
}
}
#endif //HOUND_FLOW_CHECK_HPP
