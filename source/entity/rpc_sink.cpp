//
// hound / rpc_sink.cpp.
// Created by brian on 2024-02-27.
//

#include <hound/sink/rpc_sink.hpp>
#include <hound/common/service_api.hpp>
#include <hound/type/hd_flow.hpp>

using namespace std::chrono_literals;

hd::type::RpcSink::RpcSink() {
  std::thread(&RpcSink::cleanerJob, this).detach();
  mInternalRpcServer = std::make_unique<coro_rpc::coro_rpc_server>(/*thread_num =*/opt.threads, /*port =*/opt.port);
  mInternalRpcServer->register_handler<service::sendingJob>();
  auto v = mInternalRpcServer->async_start();
  easylog::set_min_severity(easylog::Severity::WARN);
  easylog::set_async(true);
}

void hd::type::RpcSink::consumeData(ParsedData const& data) {
  if (not data.HasContent) return;
  hd_packet packet{data.mPcapHead};
  core::util::fillRawBitVec(data, packet.bitvec);
  std::scoped_lock mapLock{accessToFlowTable};
  packet_list& _existing{mFlowTable[data.mFlowKey]};
  if (flow::IsFlowReady(_existing, packet)) {
    std::scoped_lock queueLock(service::mtx_queue_access);
    service::rpc_msg_queue.emplace(data.mFlowKey, std::move(_existing));
  }
  _existing.emplace_back(std::move(packet));
  assert(_existing.size() <= opt.max_packets);
}

std::string hd::type::RpcSink::serialize(const hd_flow& flow) {
  using namespace hd::global;
  if (flow.count < opt.min_packets) return {};
  std::string message;
  struct_json::to_json(flow, message);
  return message;
}

hd::type::RpcSink::~RpcSink() {
  mIsRunning = false;
  for (auto it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
    const auto& [key, _packets] = *it;
    if (flow::_isLengthSatisfied(_packets)) {
      std::scoped_lock queueLock(service::mtx_queue_access);
      service::rpc_msg_queue.emplace(key, _packets);
    }
    it = mFlowTable.erase(it);
  }
  /// 如果 no_client  就不要一直等了
#ifndef NO_CLIENT
  while (not service::rpc_msg_queue.empty()) {
    std::this_thread::sleep_for(500ms);
  }
#endif
  hd_debug("剩余： ", mFlowTable.size());
  hd_debug("消息队列: ", service::rpc_msg_queue.size());
  mInternalRpcServer->stop();
}

void hd::type::RpcSink::cleanerJob() {
  // MEM-LEAK valgrind reports a mem-leak somewhere here, but why....
  while (mIsRunning) {
    std::this_thread::sleep_for(5s);
    std::scoped_lock lock1(accessToFlowTable);
    for (auto it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
      const auto& [key, _packets] = *it;
      if (not flow::_isTimeout(_packets)) {
        ++it;
        continue;
      }
      if (flow::_isLengthSatisfied(_packets)) {
        std::scoped_lock queueLock(service::mtx_queue_access);
        service::rpc_msg_queue.emplace(key, _packets);
      }
      it = mFlowTable.erase(it);
    }
    hd_debug("移除: ", mFlowTable.size());
  }
  hd_debug(YELLOW("函数 "), YELLOW("void cleanerJob() 结束"));
}
