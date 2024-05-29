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
extern std::atomic<int64_t>  NumBlockedFlows;
/// kafka消息一次能发的流最大条数
/// 如果太大，可能会出现kafka的相关报错：
/// local queue full, message timed out, message too large
extern int max_send_batch;
/// 单个CUDA设备一次编码的流最大条数
extern int max_encode_batch;
#if defined(BENCHMARK)
extern std::atomic<int32_t> packet_index;
extern std::atomic<int32_t> num_captured_packet;
extern std::atomic<int32_t> num_dropped_packets;
extern std::atomic<int32_t> num_consumed_packet;
extern std::atomic<int32_t> num_written_csv;
#endif
}

#endif //HOUND_GLOBAL_HPP
