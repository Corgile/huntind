//
// hound / rpc_sink.cpp.
// Created by brian on 2024-02-27.
//

#include <hound/sink/rpc_sink.hpp>
#include <hound/common/service_api.hpp>
#include <hound/type/hd_flow.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

hd::type::RpcSink::RpcSink() {
  std::thread(&RpcSink::cleanerJob, this).detach();
  static coro_rpc::coro_rpc_server server(/*thread_num =*/10, /*port =*/9000);
  server.register_handler<service::sendingJob>();
  auto v = server.async_start();
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
    // send queue 放整个 data
    service::rpc_msg_queue.emplace(data.mFlowKey, std::move(_existing));
    dbg("emplaced!");
    // std::scoped_lock lock(accessToLastArrived);
    // mLastArrived.erase(data.mFlowKey);
  }
  _existing.emplace_back(std::move(packet));
  assert(_existing.size() <= opt.max_packets);
  // std::scoped_lock lock(accessToLastArrived);
  // mLastArrived.insert_or_assign(data.mFlowKey, packet.ts_sec);
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
  hd_debug(__PRETTY_FUNCTION__);
  hd_debug(this->mFlowTable.size());
  for (auto it = mFlowTable.begin(); it not_eq mFlowTable.end();) {
    const auto& [key, _packets] = *it;
    if (flow::_isLengthSatisfited(_packets)) {
      std::scoped_lock queueLock(service::mtx_queue_access);
      service::rpc_msg_queue.emplace(key, _packets);
    }
    it = mFlowTable.erase(it);
  }
  dbg("剩余: ", mFlowTable.size());
  while (not service::rpc_msg_queue.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  dbg("剩余: ", service::rpc_msg_queue.size());
}

void hd::type::RpcSink::cleanerJob() {
  // MEM-LEAK valgrind reports a mem-leak somewhere here, but why....
  while (mIsRunning) {
    std::this_thread::sleep_for(std::chrono::seconds(opt.flowTimeout));
    if (not mIsRunning) break;
    std::scoped_lock lock1(accessToFlowTable);
    long const now = flow::timestampNow<std::chrono::seconds>();
    for (auto it = mFlowTable.begin(); it not_eq mFlowTable.end(); ++it) {
      const auto& [key, _packets] = *it;
      if (not flow::_isTimeout(_packets, now)) continue;
      if (flow::_isLengthSatisfited(_packets)) {
        std::scoped_lock queueLock(service::mtx_queue_access);
        service::rpc_msg_queue.emplace(key, _packets);
      }
      it = mFlowTable.erase(it);
    }
    hd_debug(this->mFlowTable.size());
    hd_debug(this->mSendQueue.size());
  }
  hd_debug(YELLOW("void cleanerJob() 结束"));
}
