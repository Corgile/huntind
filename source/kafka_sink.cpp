//
// Created by brian on 3/13/24.
//
#include <future>
#include <ranges>
#include <algorithm>

#include "hound/common/util.hpp"
#include "hound/common/timer.hpp"
#include "hound/sink/kafka_sink.hpp"
#include "hound/encode/flow-encode.hpp"
#include "hound/encode/transform.hpp"
#include <torch/script.h>

hd::sink::KafkaSink::KafkaSink() {
  ELOG_DEBUG << "创建 KafkaSink";
  mSendTask = std::thread(&KafkaSink::sendToKafkaTask, this);
  mCleanTask = std::thread(&KafkaSink::cleanUnwantedFlowTask, this);
}

hd::sink::KafkaSink::~KafkaSink() {
  ELOG_TRACE << __PRETTY_FUNCTION__;
  cvMsgSender.notify_all();
  mSendTask.detach();
  mCleanTask.detach();
  while (mSendQueue.size_approx() > 0) {
    std::this_thread::sleep_for(10ms);
  }
  mIsRunning = false;
  ELOG_DEBUG << CYAN("退出时， Producer [")
             << std::this_thread::get_id()
             << CYAN("] 的发送队列剩余: ")
             << this->mSendQueue.size_approx()
             << CYAN(", FlowTable 剩余 ")
             << this->mFlowTable.size();
}

/// Producer for mFlowTable
void hd::sink::KafkaSink::MakeFlow(const std::shared_ptr<raw_list>& _raw_list) {
  if (not _raw_list or _raw_list->size() <= 0) return;
  parsed_list _parsed_list{};
  _parsed_list.reserve(_raw_list->size());
  std::ranges::for_each(*_raw_list, [&_parsed_list](raw_packet const& item) {
    parsed_packet _parsed(item);
    if (not _parsed.HasContent) return;
    _parsed_list.emplace_back(_parsed);
  });
  /// 合并packet到flow
  std::scoped_lock mapLock{mtxAccessToFlowTable};
  std::ranges::for_each(_parsed_list, [this](parsed_packet const& _parsed) {
    parsed_list& _existing = mFlowTable[_parsed.mKey];
    if (util::IsFlowReady(_existing, _parsed)) {
      //      std::scoped_lock queueLock(mtxAccessToQueue);
      //      mSendQueue.emplace_back(_parsed.mKey, _existing);
      mSendQueue.enqueue({_parsed.mKey, _existing});
      //      cvMsgSender.notify_all();
      ELOG_TRACE << "加入发送队列: " << mSendQueue.size_approx();
    }
    _existing.emplace_back(_parsed);
    assert(_existing.size() <= opt.max_packets);
  });
}

void hd::sink::KafkaSink::sendToKafkaTask() {
  while (mIsRunning) {
    //    std::unique_lock lock(mtxAccessToQueue);
    //    cvMsgSender.wait(lock, [&]() -> bool {
    //      return not this->mSendQueue.empty() or not mIsRunning;
    //    });
    if (mSendQueue.size_approx() <= 0) continue;
    if (not mIsRunning) break;
    //    flow_vector _flow_list{};
    //    _flow_list.reserve(mSendQueue.size());
    //    mSendQueue.swap(_flow_list);
    //    lock.unlock();
    flow_vector _vector{};
    hd_flow flow_buffer;
    _vector.reserve(mSendQueue.size_approx());
    while (mSendQueue.try_dequeue(flow_buffer)) {
      _vector.emplace_back(flow_buffer);
    }
    auto p = std::make_shared<flow_vector>(_vector);
    auto count = std::async(std::launch::async, &hd::sink::KafkaSink::SendEncoding, this, p);
    ELOG_TRACE << __PRETTY_FUNCTION__ << ": " << count.get();
  }
  ELOG_TRACE << WHITE("函数 void sendToKafkaTask() 结束");
}

void hd::sink::KafkaSink::cleanUnwantedFlowTask() {
  while (mIsRunning) {
    std::this_thread::sleep_for(60s);
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
        //        std::scoped_lock queueLock(mtxAccessToQueue);
        //        mSendQueue.emplace_back(key, _packets);
        mSendQueue.enqueue({key, _packets});
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

int hd::sink::KafkaSink::SendEncoding(std::shared_ptr<flow_vector> const& long_flow_list) {
  std::vector<flow_vector> flow_splits;
  SplitFlows(long_flow_list, flow_splits, 1500);
  constexpr auto r = 0;
  std::ranges::for_each(flow_splits, [this](auto& item) {
    std::thread([&item, this] {
      this->_EncodeAndSend(item);
    }).join();
  });
  return r;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCallsOfFunction"

// ReSharper disable once CppDFAUnreachableFunctionCall
void hd::sink::KafkaSink::SplitFlows(std::shared_ptr<flow_vector> const& _list,
                                     std::vector<flow_vector>& output, size_t const& by) {
  std::ranges::remove_if(*_list, [](const hd_flow& item) -> bool {
    return item.count < opt.min_packets;
  });
  if (_list->size() <= by) [[likely]] {
    output.emplace_back(std::move(*_list));
    return;
  }
  output.reserve(_list->size() / by);
  while (not _list->empty()) {
    const size_t current_batch_size = std::min(by, _list->size());
    std::vector sub_vector(_list->begin(), _list->begin() + current_batch_size);
    output.emplace_back(std::move(sub_vector));
    _list->erase(_list->begin(), _list->begin() + current_batch_size);
  }
}

#pragma clang diagnostic pop

void hd::sink::KafkaSink::_EncodeAndSend(flow_vector& _flow_list) {
  std::vector<torch::Tensor> flow_data;
  flow_data.reserve(_flow_list.size());
  /// 滑动窗口
  auto transformed_view = _flow_list | std::views::transform(transform::ConvertToTensor);
  std::ranges::copy(transformed_view, std::back_inserter(flow_data));
  auto [slide_windows, flow_index_arr] = transform::BuildSlideWindow(flow_data, 5);
  /// 编码 & 合并
  const auto encoded_flows = EncodeFlowList(_flow_list, slide_windows);
  /// 记得把GPU上的数据拿到CPU中
  const auto encodings = transform::MergeFlow(encoded_flows, flow_index_arr).cpu();
  /// 拼接flowId
  std::string ordered_flow_id;
  std::ranges::for_each(_flow_list, [&ordered_flow_id](const auto& _flow) {
    ordered_flow_id.append(_flow.flowId).append("\n");
  });
  /// 发送
  const auto data_size = encodings.element_size() * encodings.numel();// calculated in byte
  scope_guard<hd::sink::ProducerUp> _guard(
    [] { return producer_pool.acquire(); },
    [](hd::sink::ProducerUp res) {
      if (not res) return;
      auto err_code = res->flush(10'000);
      if (err_code not_eq RdKafka::ERR_NO_ERROR) [[unlikely]] {
        ELOG_ERROR << "error code: " << err_code;
      }
      producer_pool.collect(std::move(res));
    }
  );
  const auto& producer = _guard.resource();
  producer->produce(
    opt.topic,
    0,
    RdKafka::Producer::RK_MSG_COPY,
    encodings.data_ptr(),
    data_size,
    ordered_flow_id.data(),
    ordered_flow_id.length(), 0, nullptr);
}

torch::Tensor hd::sink::KafkaSink::EncodeFlowList(const flow_vector& _flow_list, torch::Tensor const& slide_window) {
  std::string msg;
  size_t ns{};
  torch::Tensor encoded_flows;
  const auto count = _flow_list.size();
  {
    hd::type::Timer<std::chrono::nanoseconds> timer(ns, GREEN("<<< 编码"), msg);
    const auto modelGuard = model_pool.borrowModel();
    ELOG_TRACE << BLUE(">>> 开始编码 ") << count;
    encoded_flows = BatchEncode(modelGuard.get(), slide_window, 8192);
  }
  std::string COUNT;
  if (ns == 0) COUNT = RED("INF");
  else COUNT = std::to_string(count * 1000000000 / ns);
  ELOG_DEBUG << msg << ", 流数量: " << _flow_list.size() << ", AVG: " << COUNT << " 条/s";
  return encoded_flows
#if USE_CUDA
    .to(torch::kCUDA)
#endif
    ;
}
