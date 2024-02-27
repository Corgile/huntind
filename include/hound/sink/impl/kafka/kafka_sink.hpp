//
// Created by brian on 11/22/23.
//

#ifndef HOUND_KAFKA_HPP
#define HOUND_KAFKA_HPP

#ifdef LATENCY_TEST
#include <fstream>
#endif

#include <hound/sink/impl/kafka/kafka_config.hpp>
#include <hound/sink/impl/kafka/connection_pool.hpp>
#include <hound/sink/impl/kafka/kafka_connection.hpp>
#include <hound/sink/impl/flow_check.hpp>

namespace hd::type {
using namespace hd::entity;
using namespace hd::global;

using PacketList = std::vector<hd_packet>;

class KafkaSink final : public BaseSink {
public:
  explicit KafkaSink(std::string const& fileName)
#ifdef LATENCY_TEST
    : mTimestampLog("./flow-message-timestamp.csv", std::ios::out | std::ios::app)
#endif
  {
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
    hd_debug(__PRETTY_FUNCTION__);
    hd_debug(this->mFlowTable.size());
    delete mConnectionPool;
  }

  void consumeData(ParsedData const& data) override {
    if (not data.HasContent) return;
    hd_packet packet{data.mPcapHead};
    fillRawBitVec(data, packet.bitvec);
    std::scoped_lock mapLock{mtxAccessToFlowTable};
    PacketList& _existing{mFlowTable[data.mFlowKey]};
    if (flow::IsFlowReady(_existing, packet)) {
      std::scoped_lock queueLock(mtxAccessToQueue);
      // send queue放整个data
      mSendQueue.emplace(data.mFlowKey, std::move(_existing));
      cvMsgSender.notify_all();
      std::scoped_lock lock(mtxAccessToLastArrived);
      mLastArrived.erase(data.mFlowKey);
    }
    _existing.emplace_back(std::move(packet));
    std::scoped_lock lock(mtxAccessToLastArrived);
    mLastArrived.insert_or_assign(data.mFlowKey, packet.ts_sec);
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
      this->send(front);
    }
    hd_debug(YELLOW("void sendingJob() 结束"));
  }

  /// \brief 将mFlowTable里面超过timeout但是数量不足的flow删掉
  void cleanerJob() {
    // MEM-LEAK valgrind reports a mem-leak somewhere here, but why....
    while (mIsRunning) {
      std::this_thread::sleep_for(std::chrono::seconds(10));
      if (not mIsRunning) break;
      std::unique_lock lock1(mtxAccessToFlowTable);
      std::unique_lock lock2(mtxAccessToLastArrived);
      long const now = flow::timestampNow<std::chrono::seconds>();
      for (auto it = mLastArrived.begin(); it not_eq mLastArrived.end(); ++it) {
        const auto& [key, timestamp] = *it;
        if (now - timestamp >= opt.flowTimeout) {
          if (auto const _list = mFlowTable.at(key); _list.size() >= opt.min_packets) {
            std::scoped_lock queueLock(mtxAccessToQueue);
            mSendQueue.emplace(key, _list);
            ++it;
          }
          mFlowTable.at(key).clear();
          mFlowTable.erase(key);
          it = mLastArrived.erase(it); // 更新迭代器
        }
      }
      cvMsgSender.notify_all();
      hd_debug(this->mFlowTable.size());
      hd_debug(this->mSendQueue.size());
    }
    hd_debug(YELLOW("void cleanerJob() 结束"));
  }

  void send(const hd_flow& flow) {
    if (flow.count < opt.min_packets) return;
    std::string payload;
    struct_json::to_json(flow, payload);
    std::shared_ptr const connection{mConnectionPool->get_connection()};
#ifdef LATENCY_TEST
    {
      const auto& _front = flow.data.front();
      auto usec = std::to_string(_front.ts_usec / 1000);
      while (usec.length() < 3) usec.insert(usec.begin(), 1, '0');
      auto uniqueFlowId = flow.flowId;
      uniqueFlowId
        .append("#")
        .append(std::to_string(_front.ts_sec))
        .append(usec);
      connection->pushMessage(payload, uniqueFlowId);
      std::scoped_lock lock(mFileAccess);
      mTimestampLog << uniqueFlowId << ","
        << _front.ts_sec << usec << ","
        << flow::timestampNow<std::chrono::milliseconds>() << "\n";
    }
#else//not def LATENCY_TEST
    connection->pushMessage(payload, flow.flowId);
#endif//LATENCY_TEST
  }

  void sendTheRest() {
    if (mFlowTable.empty()) return;
    for (auto& [k, list] : mFlowTable) {
      this->send({k, list});
    }
    mFlowTable.clear();
  }

private:
  std::mutex mtxAccessToFlowTable;
  std::unordered_map<std::string, PacketList> mFlowTable;
  std::mutex mtxAccessToLastArrived;
  std::unordered_map<std::string, long> mLastArrived;

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
