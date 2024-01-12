//
// Created by brian on 12/7/23.
//

#ifndef HOUND_FLOW_CHECK_HPP
#define HOUND_FLOW_CHECK_HPP

#include <librdkafka/rdkafkacpp.h>
#include <hound/type/hd_flow.hpp>
#include <hound/common/global.hpp>

namespace flow {
using namespace hd::entity;
using namespace hd::global;
using PacketList = std::vector<hd_packet>;

static bool IsFlowReady(PacketList const& existing, hd_packet const& newPacket) {
  if (existing.size() < opt.min_packets) return false;
  return existing.size() == opt.max_packets or
    existing.back().ts_sec - newPacket.ts_sec >= opt.packetTimeout;
}

static long timestampNow() {
  auto const now = std::chrono::system_clock::now();
  // 转换为自 Unix 纪元以来的秒数
  auto const duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}
}
#endif //HOUND_FLOW_CHECK_HPP
