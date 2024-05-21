//
// Created by brian on 3/13/24.
//

#include <algorithm>

#include <hound/common/util.hpp>
#include <hound/common/timer.hpp>
#include <hound/sink/kafka_sink.hpp>
#include <hound/encode/flow-encode.hpp>
#include <hound/encode/transform.hpp>
#include <torch/script.h>

void static inline ensure_flatten_parameters(const torch::jit::Module* m) {
  for (const auto& sub_module : m->named_modules()) {
    if (not sub_module.value.type()->name()) continue;
    auto _name = sub_module.value.type()->name().value();
    if (_name == "torch::nn::LSTM" or _name == "torch::nn::GRU"
      or _name == "torch::nn::RNNBase" or _name == "torch::nn::RNN") {
      const_cast<torch::jit::Module&>(sub_module.value).run_method("flatten_parameters");
    }
  }
}

hd::sink::KafkaSink::KafkaSink() {
  mLoopTasks.reserve(opt.num_gpus);
  for (int i = 0; i < opt.num_gpus; ++i) {
    mLoopTasks.emplace_back(std::thread([this, ith = i] {
      auto mdl = torch::jit::load(opt.model_path);
      auto model = new torch::jit::Module(mdl);
      ensure_flatten_parameters(model);

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
  mSleeper.stop_sleep();
  mIsRunning = false;
  mCleanTask.join();
  for (auto& _task : mLoopTasks) {
    _task.join();
  }
  if (encoding_guard.valid()) {
    /// 相当于将 encoding_guard = std::async(...) 异步变同步
    encoding_guard.get();
  }
  easylog::set_console(true);
  ELOG_DEBUG << CYAN("退出时， Producer [")
             << std::this_thread::get_id()
             << CYAN("] 的发送队列剩余: ")
             << this->mEncodingQueue.size_approx()
             << CYAN(", FlowTable 剩余 ")
             << this->mFlowTable.size();
  easylog::set_console(false);
}

/// Producer for mFlowTable
void hd::sink::KafkaSink::MakeFlow(const shared_raw_vec& _raw_list) {
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
    if (_vector.empty()) continue;
    mNumBlockedFlows.fetch_add(_vector.size());
    ELOG_INFO << "线程 " << std::this_thread::get_id() <<  GREEN(" 新增:") << _vector.size() << RED("排队:") << mNumBlockedFlows.load();
    std::vector<std::string> ordered_id;
    ordered_id.reserve(_vector.size());
    std::ranges::for_each(_vector, [&ordered_id](const auto& _flow) {
      ordered_id.emplace_back(_flow.flowId);
    });

    auto p = std::make_shared<flow_vector>(_vector);
    auto _encoding = std::async(std::launch::async, [&] { return this->EncodeFlowList(p, model, device); });
    encoding_guard = std::async(
      std::launch::async, [this, encoding = std::move(_encoding.get()), _all_id = std::move(ordered_id)] {
        constexpr int batch_size = 1500;
        const int data_size = encoding.size(0);
        // assert(_all_id.size() == data_size);// 不一致说明EncodeFlowList内部错了
        const int batch_count = (data_size + batch_size - 1) / batch_size;
        std::vector<std::thread> _jobs;
        _jobs.reserve(batch_count);
        for (int i = 0; i < batch_count; i++) {
          _jobs.emplace_back([&, this, offset = i * batch_size]() mutable {
            std::string id;
            const auto curr_batch = std::min(batch_size, data_size - offset);
            const auto feat_ = encoding.narrow(0, offset, curr_batch);
            for (int j = 0; j < curr_batch; ++j) {
              // TODO: id太长，应适当压缩
              id.append(_all_id[offset + j]).append("\n");
            }
            this->pImpl_->send_feature_to_kafka(feat_, id);
          });
        }
        for (auto& _job : _jobs) _job.join();
      });
  }
  ELOG_INFO << CYAN("流处理任务[") << std::this_thread::get_id() << CYAN("] 结束");
}

torch::Tensor hd::sink::KafkaSink::EncodeFlowList(const shared_flow_vec& long_flow_list,
                                                  torch::jit::Module* model,
                                                  torch::Device& device) {
  std::mutex r;
  const int data_size = long_flow_list->size();
  constexpr int batch_size = 3000;
  auto const batch_count = (data_size + batch_size - 1) / batch_size;
  std::vector<torch::Tensor> result;
  std::vector<std::thread> threads;
  result.reserve(batch_count);
  threads.reserve(batch_count);

  for (size_t i = 0; i < data_size; i += batch_size) {
    threads.emplace_back([f_index = i, &long_flow_list, batch_size, this, &r, &result, &device, &model] {
      const auto _it_start = long_flow_list->begin() + f_index;
      const auto _it___end = std::min(_it_start + batch_size, long_flow_list->end());
      flow_vec_ref subvec(_it_start, _it___end);
      mNumBlockedFlows -= subvec.size();
      const auto feat = pImpl_->encode_flow_tensors(subvec, device, model);
      std::scoped_lock lock(r);
      result.emplace_back(feat);
    });
  }
  for (int i = 0; i < threads.size(); ++i) {
    threads[i].join();
  }
  return torch::concat(result, 0);
}

#pragma region Kafka Implementations

hd::type::parsed_vector
hd::sink::KafkaSink::Impl::parse_raw_packets(const shared_raw_vec& _raw_list) {
  parsed_vector _parsed_list{};
  if (not _raw_list or _raw_list->empty()) return _parsed_list;
  _parsed_list.reserve(_raw_list->size());
  std::ranges::for_each(*_raw_list, [&](raw_packet const& item) {
    parsed_packet _parsed(item);
    if (not _parsed.present()) return;
    _parsed_list.emplace_back(_parsed);
  });
  return _parsed_list;
}

void hd::sink::KafkaSink::Impl::merge_to_existing_flow(parsed_vector& _parsed_list, KafkaSink* _this) {
  /// 合并packet到flow
  std::scoped_lock mapLock{_this->mtxAccessToFlowTable};
  std::ranges::for_each(_parsed_list, [&_this](parsed_packet const& _parsed) {
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
hd::sink::KafkaSink::Impl::encode_flow_tensors(flow_vec_ref& _flow_list, torch::Device& device,
                                               torch::jit::Module* model) {
  std::string msg;
  size_t _us{};
  torch::Tensor encodings;
  const long count = _flow_list.size();
  try {
    constexpr int width = 5;
    /// TODO: encode_flow_tensors函数中， BuildSlideWindow 函数占了50%~98%的时间
    auto [slide_windows, flow_index_arr] = transform::BuildSlideWindowConcurrently(_flow_list, width, device);
    // auto [slide_windows, flow_index_arr] = transform::BuildSlideWindow(_flow_list, width, device);
    Timer<std::chrono::microseconds> timer(_us, CYAN("<<<编码"), msg);
    const auto encoded_flows = BatchEncode(model, slide_windows, 8192, 500, false);
    encodings = transform::MergeFlow(encoded_flows, flow_index_arr, device);
  //} catch (c10::Error& e) {
  } catch (...) {
    // const std::string err_msg{e.msg()};
    // const auto pos = err_msg.find_first_of('\n');
    // ELOG_ERROR << "\x1B[31;1m" << err_msg.substr(0, pos) << "\x1B[0m";
    _flow_list.clear();
    ELOG_ERROR << "使用设备CUDA:\x1B[31;1m" << device.index() << "\x1B[0m编码"<< count <<"条流失败";
    return torch::rand({1, 128});
  } 
    // catch (std::runtime_error& e) {
    // const std::string err_msg{e.what()};
    // const auto pos = err_msg.find_first_of('\n');
    // ELOG_ERROR << RED("RunTimeErr: ") << "\x1B[31;1m" << err_msg.substr(0, pos) << "\x1B[0m";
    // return torch::rand({1, 128});
    // }
  _flow_list.clear();
  ELOG_WARN << CYAN("使用设备") << "CUDA:" << device.index() << ", " << msg << GREEN(",流数量:") << count
              << ",GPU编码 ≈ " << count * 1000000 / _us << " 条/s";
  return encodings.cpu();
}

void hd::sink::KafkaSink::Impl::split_flows_by_count(
  shared_flow_vec const& _list, vec_of_flow_vec& output, size_t const& count) {
  std::ranges::remove_if(*_list, [](const hd_flow& item) -> bool {
    return item.count < opt.min_packets;
  });
  output.reserve(_list->size() / count + 1);
  while (not _list->empty()) {
    const size_t current_batch_size = std::min(count, _list->size());
    std::vector sub_vector(_list->begin(), _list->begin() + current_batch_size);
    output.emplace_back(std::move(sub_vector));
    _list->erase(_list->begin(), _list->begin() + current_batch_size);
  }
}

bool hd::sink::KafkaSink::Impl::send_feature_to_kafka(const torch::Tensor& feature, const std::string& id) {
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
    mSleeper.sleep_for(60s);
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

#pragma endregion Kafka Implementations
