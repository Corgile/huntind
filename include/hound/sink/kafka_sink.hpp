//
// Created by brian on 11/22/23.
//

#ifndef HOUND_KAFKA_HPP
#define HOUND_KAFKA_HPP

#include <torch/torch.h>

#include <hound/type/hd_flow.hpp>
#include <hound/type/parsed_packet.hpp>
#include <ylt/util/concurrentqueue.h>

namespace hd::sink {
using namespace hd::type;
using namespace hd::global;
using namespace std::chrono_literals;

using flow_map = std::unordered_map<std::string, parsed_vector>;
using flow_iter = flow_map::iterator;

using shared_flow_vec = std::shared_ptr<flow_vector>;
using vec_of_flow_vec = std::vector<flow_vector>;
using shared_raw_vec = std::shared_ptr<raw_vector>;

class KafkaSink final {
public:
  KafkaSink();

  void MakeFlow(shared_raw_vec const& _raw_list);

  ~KafkaSink();

private:
  void sendToKafkaTask();

  /// \brief 将<code>mFlowTable</code>里面超过 timeout 但是数量不足的flow删掉
  void cleanUnwantedFlowTask();

  int32_t SendEncoding(shared_flow_vec const& long_flow_list) const;

private:
  std::mutex mtxAccessToFlowTable;
  flow_map mFlowTable;

  flow_queue mEncodingQueue;

  std::vector<std::thread> mSendTasks;
  std::thread mCleanTask;

  std::atomic_bool mIsRunning{true};
  struct Impl;
  std::unique_ptr<Impl> pImpl_;
};

struct KafkaSink::Impl {
  Impl(const std::string& odel) {
    model_ = new torch::jit::Module(torch::jit::load(odel));
    model_->eval();
    for (const auto& [name, value] : model_->named_modules()) {
      ELOG_TRACE << "Module: " << name;
      try {
        auto parameters = value.parameters();
        for (auto const& param : parameters) {
          auto c = param.set_requires_grad(false);
          param.set_data(param.contiguous());
        }
        ELOG_TRACE << "Re-arranged parameters for " << name;
      } catch (c10::Error& e) {}
    }
  }

  void merge_to_existing_flow(parsed_vector&, KafkaSink*) const;
  torch::Tensor encode_flow_tensors(flow_vector& _flow_list, torch::Device& device) const;
  static parsed_vector parse_raw_packets(const shared_raw_vec& _raw_list);
  static bool send_feature_to_kafka(const torch::Tensor& feature, const std::string& id);
  static void split_flows_by_count(shared_flow_vec const&, vec_of_flow_vec&, size_t const&);
  static void split_flows_to(shared_flow_vec const&, vec_of_flow_vec&, size_t const&);

  ~Impl() {
    delete model_;
  }

private:
  torch::jit::Module* model_;
};
} // namespace hd::sink

#endif // HOUND_KAFKA_HPP
