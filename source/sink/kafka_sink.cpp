//
// hound / kafka_sink.cpp. 
// Created by brian on 2024-03-02.
//
#include <future>
#include <hound/common/util.hpp>
#include <hound/sink/impl/kafka/kafka_sink.hpp>

hd::type::KafkaSink::KafkaSink(std::string const& fileName) {
  kafka_config kafkaConfig;
  flow::LoadKafkaConfig(kafkaConfig, fileName);
  this->mConnectionPool = connection_pool::create(kafkaConfig);
  for (int i = 0; i < opt.workers; ++i) {
    std::thread(&KafkaSink::sendingJob, this).detach();
  }
  std::thread(&KafkaSink::cleanerJob, this).detach();
  mModel = torch::jit::load(opt.model_path);
  mModel.to(at::kCPU);
}

hd::type::KafkaSink::~KafkaSink() {
  hd_debug("执行函数: ", __PRETTY_FUNCTION__);
  mIsRunning = false;
  cvMsgSender.notify_all();
  // TODO:
  // 暂时不要sendTheRest(), 因为怀疑libtorch中的forward有问题。
  // 具体： 在析构发函数后, mModel 的销毁似乎会触发段错误
  // sendTheRest();
  hd_debug("flow table 中剩余: ", this->mFlowTable.size());
  delete mConnectionPool;
}

void hd::type::KafkaSink::consumeData(ParsedData const& data) {
  if (not data.HasContent) return;
  hd_packet packet{data.mPcapHead};
  hd::core::util::fillRawBitVec(data, packet.bitvec);
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

void hd::type::KafkaSink::sendingJob() {
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
  hd_debug("函数: ", YELLOW("void sendingJob() 结束"));
}

void hd::type::KafkaSink::cleanerJob() {
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
  }
  hd_debug("函数: ", YELLOW("void cleanerJob() 结束"));
}

inline bool hd::type::KafkaSink::isAttack(const hd_flow& _flow, float threshold) {
  std::vector<torch::Tensor> tensors;
  tensors.reserve(100);
  for (const auto& _packet : _flow.data) {
    int64_t _array[128];
    util::csvToArr(_packet.bitvec.c_str(), _array);
    auto single_tensor = torch::from_blob(_array, {1, 1, 128}, at::kFloat);
    tensors.emplace_back(single_tensor);
  }
  const std::vector<torch::jit::IValue> inputs{cat(tensors, 0)};
  at::Tensor output{};
  try {
    // BUG: 在退出时 forward 报错， 且不会触发 catch
    output = mModel.forward(inputs).toTensor();
  } catch (std::exception& e) {
    hd_debug("报错: ", e.what());
    return false;
  }
  const auto max_indices = argmax(output, 1);
  int attacks = 0;
  for (int i = 0; i < max_indices.size(0); ++i) {
    attacks += max_indices[i].item<long>() != 0;
  }
  auto const rate = static_cast<float>(attacks) / static_cast<float>(_flow.count);
  hd_debug("报警率: ", rate);
  return rate >= threshold;
}

void hd::type::KafkaSink::send(const hd_flow& _flow) {
  if (_flow.count < opt.min_packets) return;
  if (not isAttack(_flow, 0.4f)) return;
  std::string payload;
  struct_json::to_json(_flow, payload);
  std::shared_ptr const connection{mConnectionPool->get_connection()};
  connection->pushMessage(payload, _flow.flowId);
}

void hd::type::KafkaSink::sendTheRest() {
  hd_debug("剩余: ", mFlowTable.size());
  for (auto it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
    this->send({it->first, it->second});
    it = mFlowTable.erase(it);
  }
}
