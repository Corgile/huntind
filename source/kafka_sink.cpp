//
// Created by brian on 3/13/24.
//

#include <algorithm>
#include <hound/common/util.hpp>
#include <hound/common/timer.hpp>
#include <hound/sink/kafka_sink.hpp>
#include <hound/encode/transform.hpp>
#include <torch/script.h>

hd::sink::KafkaSink::KafkaSink() {
  ETRACE(ELOG_TRACE << "GPU个数: " << opt.num_gpus);
  mLoopTasks.reserve(opt.num_gpus);
  for (int i = 0; i < opt.num_gpus; ++i) {
    mLoopTasks.emplace_back([this, ith = (i + 1) % opt.num_gpus] {
      auto mdl = torch::jit::load(opt.model_path);
      auto model = new torch::jit::Module(mdl);
      torch::Device device(torch::kCUDA, (int8_t) ith);
      model->to(device);
      model->eval();
      ETRACE(ELOG_TRACE << "开启 LoopTask");
      LoopTask(model, device);
      delete model;
    });
  }
  mCleanTask = std::thread(&KafkaSink::cleanUnwantedFlowTask, this);
  pImpl_ = std::make_unique<Impl>();
}

hd::sink::KafkaSink::~KafkaSink() {
  mSleeper.wakeup();
  mIsRunning = false;
  mCleanTask.join();
  for (auto& _task : mLoopTasks) _task.join();
  EDEBUG(
    ELOG_DEBUG << CYAN("退出时， Producer [")
               << std::this_thread::get_id()
               << CYAN("] 的发送队列剩余: ")
               << this->doubleBufferQueue.size()
               << CYAN(", FlowTable 剩余 ")
               << this->mFlowTable.size()
  );
}

/// Producer for mFlowTable
void hd::sink::KafkaSink::MakeFlow(raw_vector& _raw_list) {
  auto _parsed_list = pImpl_->parse_raw_packets(_raw_list);
  this->pImpl_->merge_to_existing_flow(_parsed_list, this);
  ETRACE(ELOG_TRACE << __FUNCTION__ << ":" << __LINE__ << this->doubleBufferQueue.size());
}

void hd::sink::KafkaSink::LoopTask(torch::jit::Module* model, torch::Device& device) {
  while (mIsRunning) {
    std::shared_ptr current_queue = doubleBufferQueue.read();
    int const new_data_size = static_cast<int>(current_queue->size());
    ETRACE(ELOG_TRACE << "读取: " << new_data_size);
    if (new_data_size == 0) {
      std::this_thread::sleep_for(Seconds(5));
      continue;
    }

    NumBlockedFlows.fetch_add(new_data_size);
    EINFO(ELOG_INFO << GREEN("新增: ") << new_data_size << " | "
                    << RED("排队: ") << NumBlockedFlows.load());
    mTaskExecutor.AddTask([=, &device, this] {
      std::vector<std::string> key_;
      std::for_each((*current_queue).begin(), (*current_queue).end(), [&key_](auto& buf) {
        key_.emplace_back(buf.flowId);
      });

      torch::Tensor encoding = EncodeFlowList(*current_queue, model, device);

      assert(current_queue->size() == encoding.size(0));
      this->pImpl_->send_all_to_kafka(encoding, key_);
    });
  }
  EINFO(ELOG_INFO << CYAN("流处理任务[") << std::this_thread::get_id() << CYAN("] 结束"));
}

torch::Tensor hd::sink::KafkaSink::EncodeFlowList(flow_vector& data_list,
                                                  torch::jit::Module* model,
                                                  torch::Device& device) const {
  const int data_size = static_cast<int>(data_list.size());
  if (data_size < max_encode_batch) [[likely]] {
    NumBlockedFlows.fetch_sub(data_size);
    return pImpl_->encode_flow_tensors(data_list.begin(), data_list.end(), device, model);
  }
  EDEBUG(ELOG_DEBUG << CYAN("流消息太多， 采用多线程模式"));
  return pImpl_->encode_flow_concurrently(data_list.begin(), data_list.end(), device, model);
}

void hd::sink::KafkaSink::cleanUnwantedFlowTask() {
  while (mIsRunning) {
    mSleeper.sleep_for(std::chrono::seconds(opt.flowTimeout));
    std::scoped_lock lock(mtxAccessToFlowTable);
    for (auto it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
      auto& [key_, list_] = *it;
      if (list_.size() >= opt.min_packets) {
        doubleBufferQueue.enqueue({key_, list_});
        it = mFlowTable.erase(it);
      } else {
        if (util::detail::_isTimeout(list_)) {
          it = mFlowTable.erase(it);
        } else ++it;
      }
    }
  }
  ETRACE(ELOG_TRACE << WHITE("函数 void cleanUnwantedFlowTask() 结束"));
}
