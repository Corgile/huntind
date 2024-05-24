//
// Created by brian on 3/13/24.
//

#include <algorithm>
#include <malloc.h>
#include <hound/common/util.hpp>
#include <hound/common/timer.hpp>
#include <hound/sink/kafka_sink.hpp>
#include <hound/encode/flow-encode.hpp>
#include <hound/encode/transform.hpp>
#include <hound/compress_string.hpp>
#include <torch/script.h>

hd::sink::KafkaSink::KafkaSink() {
  mLoopTasks.reserve(opt.num_gpus);
  for (int i = 0; i < opt.num_gpus; ++i) {
    mLoopTasks.emplace_back(std::thread([this, ith = (i + 1) % opt.num_gpus] {
      auto mdl = torch::jit::load(opt.model_path);
      auto model = new torch::jit::Module(mdl);

      torch::Device device(torch::kCUDA, ith);
      model->to(device);
      model->eval();
      LoopTask(model, device);
      delete model;
    }));
  }
  mCleanTask = std::thread(&KafkaSink::cleanUnwantedFlowTask, this);
  pImpl_ = std::make_unique<Impl>();
}

hd::sink::KafkaSink::~KafkaSink() {
  mSleeper.wakeup();
  mIsRunning = false;
  mCleanTask.join();
  for (auto& _task : mLoopTasks) _task.join();
#ifdef HD_SHOW_LOG_DEBUG
  ELOG_DEBUG << CYAN("退出时， Producer [")
             << std::this_thread::get_id()
             << CYAN("] 的发送队列剩余: ")
             << this->mEncodingQueue.size_approx()
             << CYAN(", FlowTable 剩余 ")
             << this->mFlowTable.size();
#endif
}

/// Producer for mFlowTable
void hd::sink::KafkaSink::MakeFlow(raw_vector& _raw_list) {
  auto _parsed_list = pImpl_->parse_raw_packets(_raw_list);
  this->pImpl_->merge_to_existing_flow(_parsed_list, this);
}

void hd::sink::KafkaSink::LoopTask(torch::jit::Module* model, torch::Device& device) {
  while (mIsRunning) {
    flow_vector _vector{};
    hd_flow flow_buffer;
    _vector.reserve(mEncodingQueue.size_approx());
    while (mEncodingQueue.try_dequeue(flow_buffer)) {
      _vector.emplace_back(flow_buffer);
    }
    if (_vector.empty()) {
      std::this_thread::sleep_for(5s);
      continue;
    }

    NumBlockedFlows.fetch_add(_vector.size());
    ELOG_INFO << GREEN("新增: ") << _vector.size() << " | "
              << RED("排队: ") << NumBlockedFlows.load();

    mTaskExecutor.AddTask(
      [=, &device, this,
        long_flow_list = std::make_shared<flow_vector>(std::move(_vector))] {
        torch::Tensor encoding = EncodeFlowList(long_flow_list, model, device);
        std::vector<std::string> key_;
        key_.reserve(long_flow_list->size());
        std::ranges::for_each(*long_flow_list, [&key_](const hd_flow& _flow) {
          key_.emplace_back(_flow.flowId);
        });
        assert(long_flow_list->size() == encoding.size(0));
        this->pImpl_->send_feature_to_kafka(encoding, key_);
      });
  }
  ELOG_INFO << CYAN("流处理任务[") << std::this_thread::get_id() << CYAN("] 结束");
}

torch::Tensor hd::sink::KafkaSink::EncodeFlowList(const shared_flow_vec& long_flow_list,
                                                  torch::jit::Module* model,
                                                  torch::Device& device) const {
  const int data_size = long_flow_list->size();
  constexpr int batch_size = 3000;

  if (data_size < batch_size) [[likely]] {
    flow_vec_ref subvec(long_flow_list->begin(), long_flow_list->end());
    NumBlockedFlows -= subvec.size();
    return pImpl_->encode_flow_tensors(subvec, device, model);
  }
#pragma region Unlikely 这段代码按道理讲几乎不会被覆盖到
  ELOG_DEBUG << CYAN("流消息太多， 采用多线程模式");
  std::mutex r;
  auto const batch_count = (data_size + batch_size - 1) / batch_size;
  std::vector<torch::Tensor> result;
  std::vector<std::thread> threads;
  result.reserve(batch_count);
  threads.reserve(batch_count);

  for (size_t i = 0; i < data_size; i += batch_size) {
    threads.emplace_back([f_index = i, &long_flow_list, this, &r, &result, &device, &model] {
      const auto _it_start = long_flow_list->begin() + f_index;
      const auto _it___end = std::min(_it_start + batch_size, long_flow_list->end());
      flow_vec_ref subvec(_it_start, _it___end);
      NumBlockedFlows -= subvec.size();
      const auto feat = pImpl_->encode_flow_tensors(subvec, device, model);
      std::scoped_lock lock(r);
      result.emplace_back(feat);
    });
  }
  for (auto& _thread : threads) _thread.join();
  return torch::concat(result, 0);
#pragma endregion
}

#pragma region Kafka Implementations

hd::type::parsed_vector
hd::sink::KafkaSink::Impl::parse_raw_packets(raw_vector& _raw_list) {
  parsed_vector _parsed_list{};
  if (_raw_list.size_approx() == 0) return _parsed_list;
  _parsed_list.reserve(_raw_list.size_approx());
  raw_packet item;
  while (_raw_list.try_dequeue(item)) {
    parsed_packet _parsed(item);
    if (not _parsed.present()) continue;
    _parsed_list.emplace_back(_parsed);
  }
  return _parsed_list;
}

void hd::sink::KafkaSink::Impl::merge_to_existing_flow(parsed_vector& _parsed_list, KafkaSink* this_) {
  /// 合并packet到flow
  std::scoped_lock mapLock{this_->mtxAccessToFlowTable};
  std::ranges::for_each(_parsed_list, [this_](parsed_packet const& _parsed) {
    parsed_vector& existing = this_->mFlowTable[_parsed.mKey];
    if (util::IsFlowReady(existing)) {
      parsed_vector data{};
      data.swap(existing);
      assert(existing.empty());
      this_->mEncodingQueue.enqueue({_parsed.mKey, data});
      ELOG_TRACE << "加入编码队列: " << this_->mEncodingQueue.size_approx();
    }
    existing.emplace_back(_parsed);
    assert(existing.size() <= opt.max_packets);
  });
}

torch::Tensor
hd::sink::KafkaSink::Impl::encode_flow_tensors(flow_vec_ref& _flow_list, torch::Device& device,
                                               torch::jit::Module* model) {
  std::string msg;
  size_t _us{};
  torch::Tensor encodings;
  const long count = _flow_list.size();
  try {
    constexpr int width = 5;
    auto [slide_windows, flow_index_arr] = transform::BuildSlideWindow(_flow_list, width, device);
    Timer<std::chrono::microseconds> timer(_us, msg);
    const auto encoded_flows = BatchEncode(model, slide_windows, 8192, 500, false);
    encodings = transform::MergeFlow(encoded_flows, flow_index_arr, device);
  } catch (...) {
    _flow_list.clear();
    ELOG_ERROR << "使用设备CUDA:\x1B[31;1m" << device.index() << "\x1B[0m编码" << count << "条流失败";
    return torch::rand({1, 128});
  }
  _flow_list.clear();

  auto aligned_count = std::to_string(count);
  aligned_count.insert(0, 6 - aligned_count.size(), ' ');
  ELOG_INFO << GREEN("OK: ")
            << YELLOW("On") << "CUDA:\x1b[36;1m" << device.index() << "\x1b[0m| "
            << YELLOW("Num:") << aligned_count << "| "
            << YELLOW("Time:") << msg << "| "
            << GREEN("Avg:") << count * 1000000 / _us << " f/s";
  return encodings.cpu();
}

void
hd::sink::KafkaSink::Impl::send_feature_to_kafka(
  const torch::Tensor& feature, std::vector<std::string> const& ids) {
  /**
   * batch_size是kafka单条消息包含的【流编码最大条数】参考值
   * 如果太大，可能会出现kafka的相关报错：
   * local queue full, message timed out, message too large
   * */
  constexpr int batch_size = 1500;

  if (ids.size() <= batch_size) {
    std::string id, compressed;
    for (auto& item : ids)id.append(item).append("\n");
    /// 压缩
    zstd::compress(id, compressed);
    send_one_msg(std::move(feature), std::move(compressed));
    return;
  }

  const int data_size = feature.size(0);
  const int batch_count = (data_size + batch_size - 1) / batch_size;

  std::vector<std::thread> send_job;
  for (int i = 0; i < batch_count; i++) {
    send_job.emplace_back([=, offset = i * batch_size]() {
      std::string id, compressed;
      const auto curr_batch = std::min(batch_size, data_size - offset);
      const auto feat_ = feature.narrow(0, offset, curr_batch);
      for (int j = 0; j < curr_batch; ++j) {
        id.append(ids[offset + j]).append("\n");
      }
      /// 压缩
      zstd::compress(id, compressed);
      send_one_msg(std::move(feat_), std::move(compressed));
    });
  }
  for (auto& item : send_job) item.join();
}

bool hd::sink::KafkaSink::Impl::send_one_msg(torch::Tensor const& feature, std::string const& id) {
  const auto feature_byte_count = feature.itemsize() * feature.numel();
  ProducerManager _manager = producer_pool.acquire();
  const auto errcode = _manager->get()->produce(
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
    const auto err_code = _manager->get()->flush(5'000);
    ELOG_ERROR << err_code << " error(s)";
  }
  producer_pool.collect(std::move(_manager));
  return errcode == RdKafka::ERR_NO_ERROR;
}

void hd::sink::KafkaSink::cleanUnwantedFlowTask() {
  while (mIsRunning) {
    mSleeper.sleep_for(5s);
    std::scoped_lock lock(mtxAccessToFlowTable);
    for (flow_iter it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
      if (not util::IsFlowReady(it->second)) {
        if (util::detail::_isTimeout(it->second)) {
          it = mFlowTable.erase(it);
        } else ++it;
      } else {
        mEncodingQueue.enqueue({it->first, it->second});
        it = mFlowTable.erase(it);
      }
    }
    malloc_trim(0);
  }
  ELOG_TRACE << WHITE("函数 void cleanUnwantedFlowTask() 结束");
}

#pragma endregion Kafka Implementations
