//
// Created by brian on 11/22/23.
//

#ifndef HOUND_KAFKA_HPP
#define HOUND_KAFKA_HPP

#include <future>
#include <torch/script.h>
#include <hound/common/util.hpp>
#include <hound/sink/impl/kafka/kafka_config.hpp>
#include <hound/sink/impl/kafka/connection_pool.hpp>
#include <hound/sink/impl/kafka/kafka_connection.hpp>
#include <hound/sink/impl/flow_check.hpp>

namespace hd::type {
using namespace hd::entity;
using namespace hd::global;

using packet_list = std::vector<hd_packet>;

class KafkaSink final : public BaseSink {
public:
  explicit KafkaSink(std::string const& fileName) {
    kafka_config kafkaConfig;
    flow::LoadKafkaConfig(kafkaConfig, fileName);
    this->mConnectionPool = connection_pool::create(kafkaConfig);
    for (int i = 0; i < opt.workers; ++i) {
      std::thread(&KafkaSink::sendingJob, this).detach();
    }
    std::thread(&KafkaSink::cleanerJob, this).detach();
  }

  ~KafkaSink() override {
    hd_debug(__PRETTY_FUNCTION__);
    mIsRunning = false;
    cvMsgSender.notify_all();
    sendTheRest();
    hd_debug(this->mFlowTable.size());
    delete mConnectionPool;
  }

  void consumeData(ParsedData const& data) override {
    if (not data.HasContent) return;
    hd_packet packet{data.mPcapHead};
    core::util::fillRawBitVec(data, packet.bitvec);
    std::unique_lock mapLock{mtxAccessToFlowTable};
    packet_list& _existing{mFlowTable[data.mFlowKey]};
    if (flow::IsFlowReady(_existing, packet)) {
      std::scoped_lock queueLock(this->mtxAccessToQueue);
      this->mSendQueue.emplace(data.mFlowKey, std::move(_existing));
    }
    _existing.emplace_back(std::move(packet));
    assert(_existing.size() <= opt.max_packets);
    mapLock.unlock();
    cvMsgSender.notify_all();
  }

private:
  void sendingJob() {
    while (mIsRunning) {
      std::unique_lock lock(mtxAccessToQueue);
      cvMsgSender.wait(lock, [&]()-> bool {
        return not this->mSendQueue.empty() or not mIsRunning;
      });
      if (not mIsRunning) break;
      auto front{this->mSendQueue.front()};
      this->mSendQueue.pop();
      lock.unlock();
      /// 异步发送
      std::ignore = std::async(std::launch::async, &KafkaSink::send, this, front);
    }
    hd_debug(YELLOW("void sendingJob() 结束"));
  }

  /// \brief 将mFlowTable里面超过timeout但是数量不足的flow删掉
  void cleanerJob() {
    // MEM-LEAK valgrind reports a mem-leak somewhere here, but why....
    while (mIsRunning) {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      if (not mIsRunning) break;
      std::scoped_lock lock1(mtxAccessToFlowTable);
      for (auto it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
        const auto& [key, _packets] = *it;
        if (not flow::_isTimeout(_packets)) {
          ++it;
          continue;
        }
        if (flow::_isLengthSatisfited(_packets)) {
          std::scoped_lock queueLock(this->mtxAccessToQueue);
          this->mSendQueue.emplace(key, _packets);
        }
        it = mFlowTable.erase(it);
      }
      // hd_debug(this->mFlowTable.size());
    }
    hd_debug(YELLOW("void cleanerJob() 结束"));
  }

  bool isAttack(const hd_flow& _flow, float const threshold) {
    std::vector<torch::Tensor> tensors;
    tensors.reserve(100);
    for (const auto& _packet : _flow.data) {
      int64_t _array[128];
      util::csvToArr(_packet.bitvec.c_str(), _array);
      auto single_tensor = torch::from_blob(_array, {1, 1, 128}, at::kFloat);
      tensors.emplace_back(single_tensor);
    }
    const std::vector<torch::jit::IValue> inputs{cat(tensors, 0)};
    torch::jit::script::Module model = torch::jit::load(opt.model_path);
    model.to(at::kCPU);
    const at::Tensor output = model.forward(inputs).toTensor();
    const auto max_indices = argmax(output, 1);
    int attacks = 0;
    for (int i = 0; i < max_indices.size(0); ++i) {
      attacks += max_indices[i].item<long>() != 0;
    }
    return static_cast<float>(attacks) / static_cast<float>(_flow.count) >= threshold;
  }

  void send(const hd_flow& _flow) {
    if (_flow.count < opt.min_packets) return;
    if (not isAttack(_flow, 0.4f)) return;
    std::string payload;
    struct_json::to_json(_flow, payload);
    std::shared_ptr const connection{mConnectionPool->get_connection()};
    connection->pushMessage(payload, _flow.flowId);
  }

  void sendTheRest() {
    for (auto it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
      this->send({it->first, it->second});
      it = mFlowTable.erase(it);
    }
  }

private:
  std::mutex mtxAccessToFlowTable;
  std::unordered_map<std::string, packet_list> mFlowTable;

  std::mutex mtxAccessToQueue;
  std::queue<hd_flow> mSendQueue;
  std::condition_variable cvMsgSender;

  connection_pool* mConnectionPool;
  std::atomic_bool mIsRunning{true};
};
} // entity

#endif //HOUND_KAFKA_HPP
