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
#include <hound/common/core.hpp>
#include <hound/common/flow_check.hpp>
#include <hound/type/parsed_data.hpp>

namespace hd::type {
using namespace hd::entity;
using namespace hd::global;
using namespace std::chrono_literals;
using packet_list = std::vector<hd_packet>;

class KafkaSink final {
public:
  explicit KafkaSink(kafka_config& values,
                     std::unique_ptr<RdKafka::Conf>& _serverConf,
                     std::unique_ptr<RdKafka::Conf>& _topicConf) {
    this->pConnection = new kafka_connection(values, _serverConf, _topicConf);
    ELOG_DEBUG << "创建 kafka_connection";
    mSendTask = std::thread(&KafkaSink::sendToKafkaTask, this);
    mCleanTask = std::thread(&KafkaSink::cleanUnwantedFlowTask, this);
  }

  ~KafkaSink() {
    ELOG_TRACE << __PRETTY_FUNCTION__;
    cvMsgSender.notify_all();
    mSendTask.detach();
    mCleanTask.detach();
    while (not mSendQueue.empty()) {
      std::this_thread::sleep_for(10ms);
    }
    mIsRunning = false;
#pragma region 无需显示发送剩下的流数据
    /// 为什么加锁？ detached的线程可能造成竞态
    std::unique_lock mapLock{mtxAccessToFlowTable};
    for (flow_iter it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
      auto code = this->send({it->first, it->second});
      ELOG_TRACE << "~KafkaSink: " << code;
      it = mFlowTable.erase(it);
    }
    mapLock.unlock();
#pragma endregion
    ELOG_DEBUG << CYAN("退出时， Producer [")
               << std::this_thread::get_id()
               << CYAN("] 的发送队列剩余: ")
               << this->mSendQueue.size()
               << CYAN(", FlowTable 剩余 ")
               << this->mFlowTable.size();
    delete pConnection;
  }

  void consumeData(ParsedData const& data) {
    if (not data.HasContent) return;
    hd_packet packet{data.mPcapHead};
    core::util::fillRawBitVec(data, packet.bitvec);
    std::scoped_lock mapLock{mtxAccessToFlowTable};
    packet_list& _existing{mFlowTable[data.mFlowKey]};
    if (flow::IsFlowReady(_existing, packet)) {
      std::scoped_lock queueLock(mtxAccessToQueue);
      mSendQueue.emplace(data.mFlowKey, std::move(_existing));
      ELOG_TRACE << "加入发送队列: " << mSendQueue.size();
    }
    _existing.emplace_back(std::move(packet));
    assert(_existing.size() <= opt.max_packets);
    cvMsgSender.notify_all();
  }

private:
  void sendToKafkaTask() {
    while (mIsRunning) {
      std::unique_lock lock(mtxAccessToQueue);
      cvMsgSender.wait(lock, [&]() -> bool {
        return not this->mSendQueue.empty() or not mIsRunning;
      });
      if (not mIsRunning) break;
      while (not this->mSendQueue.empty()) {
        auto front{this->mSendQueue.front()};
        this->mSendQueue.pop();
        auto code = this->send(front);
        ELOG_TRACE << __PRETTY_FUNCTION__ << ": " << code;
      }
    }
    ELOG_TRACE << WHITE("函数 void sendToKafkaTask() 结束");
  }

  /// \brief 将<code>mFlowTable</code>里面超过 timeout 但是数量不足的flow删掉
  void cleanUnwantedFlowTask() {
    while (mIsRunning) {
      std::this_thread::sleep_for(10s);
      /// 对象析构后仍然尝试访问该对象的成员，会引发UB,
      /// 如访问悬挂指针。此处：睡一觉醒来发现this都没了，访问成员自然会SegFault
      if (not mIsRunning) break;
      std::scoped_lock lock(mtxAccessToFlowTable);
      int count = 0;
      for (flow_iter it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
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
        count++;
      }
      cvMsgSender.notify_all();
      if (not mIsRunning) break;
      if (count > 0) {
        ELOG_DEBUG << CYAN("Cleaner [")
                   << std::this_thread::get_id()
                   << CYAN("] 移除 ") << count
                   << CYAN(" 个短流, 剩余: ") << mFlowTable.size();
      }
    }
    ELOG_TRACE << WHITE("函数 void cleanUnwantedFlowTask() 结束");
  }

  int send(const hd_flow& flow) {
    if (flow.count < opt.min_packets) return -1;
    std::string payload;
    struct_json::to_json(flow, payload);
    return pConnection->pushMessage(payload, flow.flowId);
  }

private:
  std::mutex mtxAccessToFlowTable;
  std::unordered_map<std::string, packet_list> mFlowTable;
  using flow_iter = std::unordered_map<std::string, packet_list>::iterator;

  std::mutex mtxAccessToQueue;
  std::queue<hd_flow> mSendQueue;
  std::condition_variable cvMsgSender;

  std::thread mSendTask;
  std::thread mCleanTask;

  kafka_connection* pConnection;
  std::atomic_bool mIsRunning{true};
#ifdef LATENCY_TEST
  std::ofstream mTimestampLog;
  std::mutex mFileAccess;
#endif
};
} // entity

#endif //HOUND_KAFKA_HPP
