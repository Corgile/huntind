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
using packet_list = std::vector<hd_packet>;

inline bool _isTimeout(packet_list const& existing, hd_packet const& new_) {
  if(existing.empty()) return false;
  return new_.ts_sec - existing.back().ts_sec  >= opt.flowTimeout;
}

inline bool _isTimeout(packet_list const& existing, long now) {
  return now - existing.back().ts_sec >= opt.flowTimeout;
}

inline bool _isLengthSatisfited(packet_list const& existing) {
  return existing.size() >= opt.min_packets and existing.size() <= opt.max_packets;
}

static bool IsFlowReady(packet_list const& existing, hd_packet const& new_) {
  if (existing.size() == opt.max_packets) return true;
  return _isTimeout(existing, new_) and _isLengthSatisfited(existing);
}

template <typename TimeUnit = std::chrono::seconds>
static long timestampNow() {
  auto const now = std::chrono::system_clock::now();
  auto const duration = now.time_since_epoch();
  return std::chrono::duration_cast<TimeUnit>(duration).count();
}
}
#endif //HOUND_FLOW_CHECK_HPP
