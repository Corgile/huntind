//
// Created by brian on 11/22/23.
//

#ifndef HOUND_GLOBAL_HPP
#define HOUND_GLOBAL_HPP
#include <hound/type/capture_option.hpp>
#if defined(BENCHMARK)
#include <atomic>
namespace hd::global {
extern std::atomic<int32_t> packet_index;
extern std::atomic<int32_t> num_captured_packet;
extern std::atomic<int32_t> num_dropped_packets;
extern std::atomic<int32_t> num_consumed_packet;
extern std::atomic<int32_t> num_written_csv;
}
#endif


namespace hd::global {
extern type::capture_option opt;
extern std::string fillBit;
}

#endif //HOUND_GLOBAL_HPP
