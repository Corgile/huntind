//
// Created by brian on 11/22/23.
//

#ifndef HOUND_GLOBAL_HPP
#define HOUND_GLOBAL_HPP

#if defined(BENCHMARK)

#include <atomic>

#endif//-#if defined(BENCHMARK)

#include <hound/type/capture_option.hpp>
#include <hound/sink/kafka/producer_pool.hpp>

namespace hd::global {
extern type::capture_option opt;
extern ProducerPool producer_pool;
extern std::atomic_size_t NumBlockedFlows;
#if defined(BENCHMARK)
extern std::atomic<int32_t> packet_index;
extern std::atomic<int32_t> num_captured_packet;
extern std::atomic<int32_t> num_dropped_packets;
extern std::atomic<int32_t> num_consumed_packet;
extern std::atomic<int32_t> num_written_csv;
#endif
}

#endif //HOUND_GLOBAL_HPP
