//
// Created by brian on 3/13/24.
//
#include <future>
#include <ranges>
#include <algorithm>

#include <hound/common/util.hpp>
#include <hound/common/timer.hpp>
#include <hound/sink/kafka_sink.hpp>
#include <hound/encode/flow-encode.hpp>
#include <hound/encode/transform.hpp>
#include <torch/script.h>

hd::sink::KafkaSink::KafkaSink() {
  ELOG_DEBUG << "创建 KafkaSink";
  mSendTasks.reserve(4);
  mSendTasks.emplace_back(std::thread(&KafkaSink::sendToKafkaTask, this));
  mSendTasks.emplace_back(std::thread(&KafkaSink::sendToKafkaTask, this));
  mSendTasks.emplace_back(std::thread(&KafkaSink::sendToKafkaTask, this));
  mSendTasks.emplace_back(std::thread(&KafkaSink::sendToKafkaTask, this));
  mCleanTask = std::thread(&KafkaSink::cleanUnwantedFlowTask, this);
}

hd::sink::KafkaSink::~KafkaSink() {
  // mSendTask.detach();
  for (auto& _send_task : mSendTasks) {
    _send_task.detach();
  }
  mCleanTask.detach();
  while (mEncodingQueue.size_approx() > 0) {
    std::this_thread::sleep_for(10ms);
  }
  mIsRunning = false;
  ELOG_DEBUG << CYAN("退出时， Producer [")
             << std::this_thread::get_id()
             << CYAN("] 的发送队列剩余: ")
             << this->mEncodingQueue.size_approx()
             << CYAN(", FlowTable 剩余 ")
             << this->mFlowTable.size();
}

/// Producer for mFlowTable
void hd::sink::KafkaSink::MakeFlow(const shared_raw_vec& _raw_list) {
  auto _parsed_list = pImpl_->parse_raw_packets(_raw_list);
  this->pImpl_->merge_to_existing_flow(_parsed_list, this);
}

void hd::sink::KafkaSink::sendToKafkaTask() {
  while (mIsRunning) {
    if (not mIsRunning) break;
    flow_vector _vector{};
    hd_flow flow_buffer;
    _vector.reserve(mEncodingQueue.size_approx());
    while (mEncodingQueue.try_dequeue(flow_buffer)) {
      _vector.emplace_back(flow_buffer);
    }
    if (_vector.empty()) continue;
    ELOG_WARN << YELLOW("编码队列") << RED("↓") << _vector.size()
              << YELLOW(",剩余") << mEncodingQueue.size_approx();
    auto p = std::make_shared<flow_vector>(_vector);
    // auto count = this->SendEncoding(p);
    auto count = std::async(std::launch::async, [&] { return this->SendEncoding(p); });
    ELOG_TRACE << "成功发送 " << count.get() << "条消息";
  }
  ELOG_TRACE << WHITE("函数 void sendToKafkaTask() 结束");
}

/// 瓶颈
void hd::sink::KafkaSink::cleanUnwantedFlowTask() {
  while (mIsRunning) {
    std::this_thread::sleep_for(60s);
    if (not mIsRunning) break;
    std::scoped_lock lock(mtxAccessToFlowTable);
    int count = 0, transferred = 0;
    for (flow_iter it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
      auto& [key, _packets] = *it;
      if (not util::detail::_isTimeout(_packets)) {
        ++it;
        continue;
      }
      if (util::detail::_checkLength(_packets)) {
        mEncodingQueue.enqueue({key, _packets});
        transferred++;
      }
      it = mFlowTable.erase(it);
      count++;
    }
    if (not mIsRunning) break;
    if (count > 0) {
      ELOG_DEBUG << CYAN("FlowTable") << RED("-") << count
                 << "=" << GREEN("EncodeQueue") << GREEN("+") << transferred
                 << RED("Drop(") << count - transferred << RED(")")
                 << BLUE(". Queue:")
                 << mEncodingQueue.size_approx()
                 << BLUE(",Table")
                 << mFlowTable.size();
    }
  }
  ELOG_TRACE << WHITE("函数 void cleanUnwantedFlowTask() 结束");
}

// ReSharper disable once CppDFAUnreachableFunctionCall
int32_t hd::sink::KafkaSink::SendEncoding(shared_flow_vec const& long_flow_list) const {
  std::atomic_int32_t res{0};
  vec_of_flow_vec flow_splits;
  this->pImpl_->split_flows(long_flow_list, flow_splits, 1500);
  std::vector<std::thread> encoding_jobs;
  encoding_jobs.reserve(flow_splits.size());
  for (int i = 0; i < flow_splits.size(); ++i) {
    const int _index = i % 6;
    const int _f_index = i;
    encoding_jobs.emplace_back([&flow_splits, _f_index, _index, this, &res] {
      auto& _flow_list = flow_splits.at(_f_index);
      constexpr int _cuda[] = {0, 1, 2, 4, 5, 6, 7};//because 3 is down.
      auto _device = torch::Device(torch::kCUDA, _cuda[_index]);
      const auto feat = pImpl_->encode_flow_tensors(_flow_list, _device);
      if (feat.equal(torch::empty({1}))) return;
      /// 拼接flowId
      std::string ordered_flow_id;
      std::ranges::for_each(_flow_list, [&ordered_flow_id](const auto& _flow) {
        ordered_flow_id.append(_flow.flowId).append("\n");
      });
      res += this->pImpl_->send_feature_to_kafka(feat, ordered_flow_id);
    });
  }
  for (auto& _encoding_job : encoding_jobs) {
    _encoding_job.join();
  }
  return res;
}

#pragma region Kafka Implementations

hd::type::parsed_vector
hd::sink::KafkaSink::Impl::
parse_raw_packets(const shared_raw_vec& _raw_list) {
  parsed_vector _parsed_list{};
  if (not _raw_list or _raw_list->size() <= 0) return _parsed_list;
  _parsed_list.reserve(_raw_list->size());
  std::ranges::for_each(*_raw_list, [&](raw_packet const& item) {
    parsed_packet _parsed(item);
    if (not _parsed.present()) return;
    _parsed_list.emplace_back(_parsed);
  });
  return _parsed_list;
}

void hd::sink::KafkaSink::Impl::merge_to_existing_flow(parsed_vector& _parsed_list, KafkaSink* _this) const {
  /// 合并packet到flow
  std::scoped_lock mapLock{_this->mtxAccessToFlowTable};
  std::ranges::for_each(_parsed_list, [&_this](auto const& _parsed) {
    parsed_vector& _existing = _this->mFlowTable[_parsed.mKey];
    if (util::IsFlowReady(_existing, _parsed)) {
      parsed_vector data{};
      data.swap(_existing);
      _this->mEncodingQueue.enqueue({_parsed.mKey, data});
      ELOG_TRACE << "加入编码队列: " << _this->mEncodingQueue.size_approx();
    }
    _existing.emplace_back(_parsed);
    assert(_existing.size() <= opt.max_packets);
  });
}

torch::Tensor
hd::sink::KafkaSink::Impl::encode_flow_tensors(flow_vector& _flow_list, torch::Device& device) {
  std::string msg;
  size_t _us{};
  torch::Tensor encodings;
  try {
    auto [slide_windows, flow_index_arr] = transform::BuildSlideWindow(_flow_list, 5, device);
    Timer<std::chrono::microseconds> timer(_us, GREEN("编码"), msg);
    const auto encoded_flows = BatchEncode(slide_windows, 8192, 500, false, device);
    encodings = transform::MergeFlow(encoded_flows, flow_index_arr, device);
  } catch (c10::Error& e) {
    const std::string err_msg{e.msg()};
    const auto pos = err_msg.find_first_of('\n');
    ELOG_ERROR << "\x1B[31;1m" << err_msg.substr(0, pos) << "\x1B[0m";
    return torch::empty({1});
  } catch (std::runtime_error& e) {
    const std::string err_msg{e.what()};
    const auto pos = err_msg.find_first_of('\n');
    ELOG_ERROR << RED("RunTimeErr: ") << "\x1B[31;1m" << err_msg.substr(0, pos) << "\x1B[0m";
    return torch::empty({1});
  }
  const long count = _flow_list.size();
  ELOG_DEBUG << CYAN("设备") << "CUDA:" << device.index() << ", "
              << msg << GREEN(",流数量:") << count
              << ",本批次GPU编码 ≈ " << count * 1000000 / _us << " 条/s";
  return encodings.cpu();
}

void hd::sink::KafkaSink::Impl::split_flows(shared_flow_vec const& _list, vec_of_flow_vec& output, size_t const& by) {
  std::ranges::remove_if(*_list, [](const hd_flow& item) -> bool {
    return item.count < opt.min_packets;
  });
  output.reserve(_list->size() / by + 1);
  while (not _list->empty()) {
    const size_t current_batch_size = std::min(by, _list->size());
    std::vector sub_vector(_list->begin(), _list->begin() + current_batch_size);
    output.emplace_back(std::move(sub_vector));
    _list->erase(_list->begin(), _list->begin() + current_batch_size);
  }
}

bool hd::sink::KafkaSink::Impl::send_feature_to_kafka(const torch::Tensor& feature, const std::string& id) {
  /// 发送
  const auto feature_byte_count = feature.element_size() * feature.numel();
  ProducerUp producer = producer_pool.acquire();
  const auto errcode = producer->produce(
    opt.topic,
    // 不指定分区, 由patiotionerCB指定。
    RdKafka::Topic::PARTITION_UA,
    // 将payload复制一份给rdkafka, 其内存将由rdkafka管理
    RdKafka::Producer::RK_MSG_COPY,
    feature.data_ptr(),
    feature_byte_count,
    id.data(),// need compress
    id.length(), 0, nullptr);
  if (errcode == RdKafka::ERR_REQUEST_TIMED_OUT) {
    /// flush: block wait.
    const auto err_code = producer->flush(10'000);
    ELOG_ERROR << err_code << " error(s)";
  }
  producer_pool.collect(std::move(producer));
  return errcode == RdKafka::ERR_NO_ERROR;
}
#pragma endregion Kafka Implementations
