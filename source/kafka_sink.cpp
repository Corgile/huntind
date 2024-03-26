//
// Created by brian on 3/13/24.
//
#include <my-timer.hpp>
#include "hound/common/util.hpp"
#include "hound/sink/kafka_sink.hpp"

#include "hound/encode/flow-encode.hpp"
#include "hound/encode/transform.hpp"

hd::sink::KafkaSink::KafkaSink(const kafka_config& values,
                               const RdConfUptr& _serverConf,
                               const RdConfUptr& _topicConf) {
  this->pConnection = new kafka_connection(values, _serverConf, _topicConf);
  ELOG_DEBUG << "创建 kafka_connection";
  mSendTask = std::thread(&KafkaSink::sendToKafkaTask, this);
  mCleanTask = std::thread(&KafkaSink::cleanUnwantedFlowTask, this);
  mModel = load_model();
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
// TODO: std::vector<raw_packet> ~ std::swap
void hd::sink::KafkaSink::consume_data(raw_packet const& raw) {
  parsed_packet _packet(raw);
  if (not _packet.HasContent) return;
  std::scoped_lock mapLock{mtxAccessToFlowTable};
  packet_list& _existing{mFlowTable[_packet.mKey]};
  if (util::IsFlowReady(_existing, _packet)) {
    std::scoped_lock queueLock(mtxAccessToQueue);
    mSendQueue.emplace_back(_packet.mKey, _existing);
    ELOG_TRACE << "加入发送队列: " << mSendQueue.size();
  }
  _existing.emplace_back(_packet);//copy
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
    std::vector<hd_flow> flow_list;
    flow_list.reserve(mSendQueue.size());
    mSendQueue.swap(flow_list);
    lock.unlock();
    auto code = this->send(flow_list);
    ELOG_TRACE << __PRETTY_FUNCTION__ << ": " << code;
  }
  ELOG_TRACE << WHITE("函数 void sendToKafkaTask() 结束");
}

void hd::sink::KafkaSink::cleanUnwantedFlowTask() {
  while (mIsRunning) {
    std::this_thread::sleep_for(60s);
    /// 对象析构后仍然尝试访问该对象的成员，会引发UB,
    /// 如访问悬挂指针。此处：睡一觉醒来发现this都没了，访问成员自然会SegFault
    if (not mIsRunning) break;
    std::scoped_lock lock(mtxAccessToFlowTable);
    int count = 0;
    for (flow_iter it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
      auto& [key, _packets] = *it;
      if (not util::detail::_isTimeout(_packets)) {
        ++it;
        continue;
      }
      if (util::detail::_checkLength(_packets)) {
        std::scoped_lock queueLock(mtxAccessToQueue);
        mSendQueue.emplace_back(key, _packets);
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

// ReSharper disable once CppDFAUnreachableFunctionCall
int hd::sink::KafkaSink::send(std::vector<hd_flow>& long_flow_list) {
  std::ranges::remove_if(long_flow_list, [](const hd_flow& item)-> bool {
    return item.count < opt.min_packets;
  });
  std::vector<std::vector<hd_flow>> flow_splits;
  [&flow_splits, &long_flow_list](const size_t by)-> void {
    flow_splits.reserve(long_flow_list.size() / by);
    while (not long_flow_list.empty()) {
      const size_t current_batch_size = std::min(by, long_flow_list.size());
      std::vector sub_vector(long_flow_list.begin(), long_flow_list.begin() + current_batch_size);
      flow_splits.emplace_back(std::move(sub_vector));
      long_flow_list.erase(long_flow_list.begin(), long_flow_list.begin() + current_batch_size);
    }
  }(1500);
  auto r = 0;
  for (const auto& _flow_list : flow_splits) {
    std::vector<torch::Tensor> flow_data;
    for (const auto& msg : _flow_list) {
      flow_data.push_back(transform::convert_to_npy(msg));
    }
    /// flow_data: in shape of (num_flows, num_flow_packets, packet_length)
    auto [slide_windows, flow_index_arr] = transform::build_slide_window(flow_data, 5);
    ELOG_TRACE << BLUE(">>> 开始编码 ") << _flow_list.size();
    torch::Tensor encoded_flows;
    std::string msg;
    {
      xhl::Timer timer(GREEN("<<< 编码"), msg);
      encoded_flows = batch_model_encode(mModel, slide_windows, 8192);
    }
    ELOG_DEBUG << msg << ", 流数量: " << _flow_list.size();
    const auto encodings = transform::merge_flow(encoded_flows, flow_index_arr).detach();
    std::string ordered_flow_id; // 与encodings中的每一个encoding对应
    for (const auto& _flow : _flow_list) {
      ordered_flow_id.append(_flow.flowId).append("\n");
    }
    const auto data_size = encodings.element_size() * encodings.numel();// calculated in byte
    r += pConnection->pushMessage(encodings.data_ptr(), data_size, ordered_flow_id);
  }
  return r;
}
