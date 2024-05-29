//
// hound-torch / _kafka_impl.cpp
// Created by brian on 2024-05-27.
//
#include <hound/sink/kafka_sink.hpp>
#include <hound/common/util.hpp>
#include <hound/encode/flow-encode.hpp>
#include <hound/encode/transform.hpp>
#include <hound/common/timer.hpp>

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
      this_->doubleBufferQueue.enqueue({_parsed.mKey, data});
#ifdef HD_LOG_LEVEL_TRACE
      ELOG_TRACE << "加入编码队列: " << this_->doubleBufferQueue.size();
#endif
    }
    existing.emplace_back(_parsed);
    assert(existing.size() <= opt.max_packets);
  });
}

torch::Tensor
hd::sink::KafkaSink::Impl::
encode_flow_tensors(flow_vector::const_iterator _begin,
                    flow_vector::const_iterator _end,
                    torch::Device& device,
                    torch::jit::Module* model) {
  std::string msg;
  size_t _us{};
  torch::Tensor encodings;
  const long count = std::distance(_begin, _end);
  try {
    constexpr int width = 5;
    auto [sld_wind, flow_idx_arr] =
      transform::BuildSlideWindow({_begin, _end}, width, device);
    Timer<std::chrono::microseconds> timer(_us, msg);
    const auto encoded_flows = BatchEncode(model, sld_wind, 8192);
    encodings = transform::MergeFlow(encoded_flows, flow_idx_arr, device);
  } catch (...) {
    ELOG_ERROR << "使用设备CUDA:\x1B[31;1m" << device.index() << "\x1B[0m编码" << count << "条流失败";
    return torch::rand({count, 1, 128});
  }
  auto aligned_count = std::to_string(count);
  aligned_count.insert(0, 6 - aligned_count.size(), ' ');
#ifdef HD_LOG_LEVEL_INFO
  ELOG_INFO << GREEN("OK: ")
            << YELLOW("On") << "CUDA:\x1b[36;1m" << device.index() << "\x1b[0m| "
            << YELLOW("Num:") << aligned_count << "| "
            << YELLOW("Time:") << msg << "| "
            << GREEN("Avg:") << count * 1000000 / _us << " f/s";
#endif
  return encodings.cpu();
}

void
hd::sink::KafkaSink::Impl::send_all_to_kafka(const torch::Tensor& feature, std::vector<std::string> const& ids) {
  auto const data_size = ids.size();
  if (data_size <= max_send_batch) {
    send_one_msg(feature, ids);
    return;
  }
  send_concurrently(feature, ids);
}

void hd::sink::KafkaSink::Impl::send_concurrently(torch::Tensor const& feature, std::vector<std::string> const& ids) {

  int const data_size = static_cast<int>(ids.size());
  int const batch_count = (data_size + max_send_batch - 1) / max_send_batch;
  std::vector<std::thread> send_job;
  for (int i = 0; i < batch_count; i++) {
    send_job.emplace_back([=, offset = i * max_send_batch]() {
      const auto curr_batch = std::min(max_send_batch, data_size - offset);
      const auto feat_ = feature.narrow(0, offset, curr_batch);
      std::vector _ids(ids.begin() + offset, ids.begin() + offset + curr_batch);
      send_one_msg(feat_, _ids);
    });
  }
  for (auto& item : send_job) item.join();
}


bool hd::sink::KafkaSink::Impl::send_one_msg(torch::Tensor const& feature, std::vector<std::string> const& ids) {
  std::string id;//, compressed;
  for (auto& item : ids) id.append(item).append("\n");
  /// 压缩： 在配置 \p compression.type  参数后是否还有必要？
  // zstd::compress(id, compressed);
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

torch::Tensor hd::sink::KafkaSink::Impl::
encode_flow_concurrently(flow_vector::const_iterator _begin,
                         flow_vector::const_iterator _end,
                         torch::Device& device,
                         torch::jit::Module* model) {
  const long data_size = std::distance(_begin, _end);
  auto const batch_count = (data_size + max_encode_batch - 1) / max_encode_batch;
  std::vector<torch::Tensor> result(batch_count);
  std::vector<std::thread> threads;
  threads.reserve(batch_count);
  for (int batch_index = 0; batch_index < batch_count; ++batch_index) {
    int f_index = batch_index * max_encode_batch;
    threads.emplace_back([&, f_index, batch_index] {
      const auto _it_beg = _begin + f_index;
      const auto _it_end = std::min(_it_beg + max_encode_batch, _end);
      NumBlockedFlows.fetch_sub(std::distance(_it_beg, _it_end));
      const auto feat = encode_flow_tensors(_it_beg, _it_end, device, model);
      result[batch_index] = feat;
    });
  }
  for (auto& _thread : threads) _thread.join();
  return torch::concat(result, 0);
}


#pragma endregion Kafka Implementations
