//
// Created by brian on 11/22/23.
//

#ifndef HOUND_KAFKA_HPP
#define HOUND_KAFKA_HPP

#ifdef LATENCY_TEST
  #include <fstream>
#endif

#include <future>
#include <hound/sink/impl/kafka/kafka_config.hpp>
#include <hound/sink/impl/kafka/connection_pool.hpp>
#include <hound/sink/impl/kafka/kafka_connection.hpp>
#include <hound/common/flow_check.hpp>

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
    mIsRunning = false;
    cvMsgSender.notify_all();
#pragma region 发送剩下的流数据
    for (auto it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
      auto code = this->send({it->first, it->second});
      hd_debug(GREEN("~KafkaSink: "), code);
      it = mFlowTable.erase(it);
    }
    hd_debug("剩余 ", this->mFlowTable.size());
#pragma endregion
    delete mConnectionPool;
  }

  void consumeData(ParsedData const& data) override {
    if (not data.HasContent) return;
    hd_packet packet{data.mPcapHead};
    core::util::fillRawBitVec(data, packet.bitvec);
    std::scoped_lock mapLock{mtxAccessToFlowTable};
    packet_list& _existing{mFlowTable[data.mFlowKey]};
    if (flow::IsFlowReady(_existing, packet)) {
      std::scoped_lock queueLock(mtxAccessToQueue);
      mSendQueue.emplace(data.mFlowKey, std::move(_existing));
    }
    _existing.emplace_back(std::move(packet));
    assert(_existing.size() <= opt.max_packets);
    cvMsgSender.notify_all();
  }

private:
  void sendingJob() {
    while (mIsRunning) {
      std::unique_lock lock(mtxAccessToQueue);
      cvMsgSender.wait(lock, [&]() -> bool {
        return not this->mSendQueue.empty() or not mIsRunning;
      });
      if (not mIsRunning) break;
      while (not this->mSendQueue.empty()) {
        auto front{this->mSendQueue.front()};
        this->mSendQueue.pop();
        auto future = std::async(std::launch::async, &KafkaSink::send, this, front);
        hd_debug(GREEN("SendingJob: "), future.get());
      }
      lock.unlock();
    }
    hd_debug(YELLOW("函数 "), YELLOW("void sendingJob() 结束"));
  }

  /// \brief 将mFlowTable里面超过timeout但是数量不足的flow删掉
  void cleanerJob() {
    // MEM-LEAK valgrind reports a mem-leak somewhere here, but why....
    while (mIsRunning) {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      std::scoped_lock lock1(mtxAccessToFlowTable);
      for (auto it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
        const auto& [key, _packets] = *it;
        if (not flow::_isTimeout(_packets)) {
          ++it;
          continue;
        }
        if (flow::_isLengthSatisfied(_packets)) {
          std::scoped_lock queueLock(mtxAccessToQueue);
          mSendQueue.emplace(key, _packets);
        }
        it = mFlowTable.erase(it);
      }
      cvMsgSender.notify_all();
      if (not mIsRunning) break;
      hd_debug("移除不好的flow: ", mFlowTable.size());
    }
    hd_debug("函数 ", YELLOW("void cleanerJob() 结束"));
  }

  int send(const hd_flow& flow) {
    if (flow.count < opt.min_packets) return -1;
    std::string payload;
    struct_json::to_json(flow, payload);
    std::shared_ptr const connection{mConnectionPool->get_connection()};
    return connection->pushMessage(payload, flow.flowId);
  }

private:
  std::mutex mtxAccessToFlowTable;
  std::unordered_map<std::string, packet_list> mFlowTable;

  std::mutex mtxAccessToQueue;
  std::queue<hd_flow> mSendQueue;
  std::condition_variable cvMsgSender;

  connection_pool* mConnectionPool;
  std::atomic_bool mIsRunning{true};
#ifdef LATENCY_TEST
  std::ofstream mTimestampLog;
  std::mutex mFileAccess;
#endif
};
} // entity

#endif //HOUND_KAFKA_HPP
