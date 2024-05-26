//
// Created by brian on 11/22/23.
//

#ifndef HOUND_KAFKA_HPP
#define HOUND_KAFKA_HPP

#include <future>
#include <torch/torch.h>

#include <hound/type/hd_flow.hpp>
#include <hound/type/parsed_packet.hpp>
#include <ylt/util/concurrentqueue.h>
#include <hound/interruptible_sleep.hpp>
#include <hound/task_executor.hpp>

#include <hound/double_buffered_queue.hpp>

namespace hd::sink {
using namespace hd::type;
using namespace hd::global;
using namespace std::chrono_literals;

// using flow_map = std::map<std::string, parsed_vector>;
using flow_map = std::pmr::map<std::string, parsed_vector>;
using flow_iter = flow_map::iterator;

using shared_flow_vec = std::shared_ptr<flow_vector>;
using vec_of_flow_vec = std::vector<flow_vector>;
using shared_raw_vec = std::shared_ptr<raw_vector>;

class KafkaSink final {
public:
  KafkaSink();

  void MakeFlow(raw_vector& _raw_list);

  ~KafkaSink();

private:
  void LoopTask(torch::jit::Module* model, torch::Device& device);

  /// \brief 将<code>mFlowTable</code>里面超过 timeout 但是数量不足的flow删掉
  void cleanUnwantedFlowTask();

  // [[deprecated]]
  // int32_t SendEncoding(shared_flow_vec const& long_flow_list) const;

  torch::Tensor EncodeFlowList(flow_vector& data_list, torch::jit::Module* model, torch::Device& device) const;

private:
  std::mutex mtxAccessToFlowTable;
  flow_map mFlowTable;

  // flow_queue mEncodingQueue[2];
  // std::atomic<int> current{0};

  DoubleBufferQueue <hd_flow> doubleBufferQueue;

  std::vector<std::thread> mLoopTasks;
  std::thread mCleanTask;

  std::atomic_bool mIsRunning{true};

  InterruptibleSleep mSleeper;
  TaskExecutor mTaskExecutor;
  struct Impl;
  std::unique_ptr<Impl> pImpl_;
};

struct KafkaSink::Impl {
  static void merge_to_existing_flow(parsed_vector&, KafkaSink*);

  static torch::Tensor
  encode_flow_tensors(flow_vector::const_iterator _begin,
                      flow_vector::const_iterator _end,
                      torch::Device& device,
                      torch::jit::Module* model);

  static torch::Tensor
  encode_flow_concurrently(flow_vector::const_iterator _begin,
                      flow_vector::const_iterator _end,
                      torch::Device& device,
                      torch::jit::Module* model);

  static parsed_vector parse_raw_packets(raw_vector& _raw_list);

  static void send_all_to_kafka(const torch::Tensor& feature, std::vector<std::string> const& ids);

private:
  static void send_concurrently(const torch::Tensor& feature, std::vector<std::string> const& ids);

  static bool send_one_msg(torch::Tensor const& feature, std::vector<std::string> const& id);
};
} // namespace hd::sink

#endif // HOUND_KAFKA_HPP
