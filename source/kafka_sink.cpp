//
// Created by brian on 3/13/24.
//
#include "hound/common/util.hpp"
#include "hound/sink/kafka_sink.hpp"
#include "hound/encoding/flow-encode.hpp"

hd::sink::KafkaSink::KafkaSink(hd::type::kafka_config& values,
                               RdConfUptr& _serverConf,
                               RdConfUptr& _topicConf) {
  this->pConnection = new kafka_connection(values, _serverConf, _topicConf);
  ELOG_DEBUG << "创建 kafka_connection";
  mSendTask = std::thread(&KafkaSink::sendToKafkaTask, this);
  mCleanTask = std::thread(&KafkaSink::cleanUnwantedFlowTask, this);
}

hd::sink::KafkaSink::~KafkaSink() {
  ELOG_TRACE << __PRETTY_FUNCTION__;
  cvMsgSender.notify_all();
  mSendTask.detach();
  mCleanTask.detach();
  while (not mSendQueue.empty()) {
    std::this_thread::sleep_for(10ms);
  }
  mIsRunning = false;
  ELOG_DEBUG << CYAN("退出时， Producer [")
             << std::this_thread::get_id()
             << CYAN("] 的发送队列剩余: ")
             << this->mSendQueue.size()
             << CYAN(", FlowTable 剩余 ")
             << this->mFlowTable.size();
  delete pConnection;
}

void hd::sink::KafkaSink::consumeData(hd::type::ParsedData const& data) {
  if (not data.HasContent) return;
  hd_packet packet{data.mPcapHead};
  // hd::util::fillRawBitVec(data, packet.raw);
  std::scoped_lock mapLock{mtxAccessToFlowTable};
  packet_list& _existing{mFlowTable[data.mFlowKey]};
  if (hd::util::IsFlowReady(_existing, packet)) {
    std::scoped_lock queueLock(mtxAccessToQueue);
    mSendQueue.emplace(data.mFlowKey, std::move(_existing));
    ELOG_TRACE << "加入发送队列: " << mSendQueue.size();
  }
  _existing.emplace_back(std::move(packet));
  assert(_existing.size() <= opt.max_packets);
  cvMsgSender.notify_all();
}

void hd::sink::KafkaSink::sendToKafkaTask() {
  while (mIsRunning) {
    std::unique_lock lock(mtxAccessToQueue);
    cvMsgSender.wait(lock, [&]() -> bool {
      return not this->mSendQueue.empty() or not mIsRunning;
    });
    if (not mIsRunning) break;
    auto front{this->mSendQueue.front()};
    this->mSendQueue.pop();
    lock.unlock();
    auto code = this->send(front);
    ELOG_TRACE << __PRETTY_FUNCTION__ << ": " << code;
  }
  ELOG_TRACE << WHITE("函数 void sendToKafkaTask() 结束");
}

void hd::sink::KafkaSink::cleanUnwantedFlowTask() {
  while (mIsRunning) {
    std::this_thread::sleep_for(10s);
    /// 对象析构后仍然尝试访问该对象的成员，会引发UB,
    /// 如访问悬挂指针。此处：睡一觉醒来发现this都没了，访问成员自然会SegFault
    if (not mIsRunning) break;
    std::scoped_lock lock(mtxAccessToFlowTable);
    int count = 0;
    for (FlowIter it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
      const auto& [key, _packets] = *it;
      if (not hd::util::detail::_isTimeout(_packets)) {
        ++it;
        continue;
      }
      if (hd::util::detail::_checkLength(_packets)) {
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

int inline hd::sink::KafkaSink::send(hd::type::hd_flow const& flow) {
  if (flow.count < opt.min_packets) return -1;
  auto tensor = encode(flow);
  tensor.print();
  return 0;
}
