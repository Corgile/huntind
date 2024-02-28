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
}

void hd::type::RpcSink::consumeData(ParsedData const& data) {
  if (not data.HasContent) return;
  hd_packet packet{data.mPcapHead};
  core::util::fillRawBitVec(data, packet.bitvec);
  std::scoped_lock mapLock{mtxAccessToFlowTable};
  packet_list& _existing{mFlowTable[data.mFlowKey]};
  if (flow::IsFlowReady(_existing, packet)) {
    std::scoped_lock queueLock(service::mtx_queue_access);
    // send queue 放整个 data
    service::rpc_msg_queue.emplace(data.mFlowKey, std::move(_existing));
    dbg("emplaced!");
    std::scoped_lock lock(mtxAccessToLastArrived);
    mLastArrived.erase(data.mFlowKey);
  }
  _existing.emplace_back(std::move(packet));
  std::scoped_lock lock(mtxAccessToLastArrived);
  mLastArrived.insert_or_assign(data.mFlowKey, packet.ts_sec);
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
}

void hd::type::RpcSink::cleanerJob() {
  // MEM-LEAK valgrind reports a mem-leak somewhere here, but why....
  while (mIsRunning) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    if (not mIsRunning) break;
    std::scoped_lock lock1(mtxAccessToFlowTable);
    std::scoped_lock lock2(mtxAccessToLastArrived);
    long const now = flow::timestampNow<std::chrono::seconds>();
    for (auto it = mLastArrived.begin(); it not_eq mLastArrived.end(); ++it) {
      const auto& [key, timestamp] = *it;
      // TODO  线程不安全
      if (not mFlowTable.contains(key)) {
        mLastArrived.erase(key);
        continue;
      }
      if (now - timestamp >= opt.flowTimeout) {
        if (auto const& _list = mFlowTable.at(key); _list.size() >= opt.min_packets) {
          std::scoped_lock queueLock(service::mtx_queue_access);
          service::rpc_msg_queue.emplace(key, _list);
          ++it;
        }
        mFlowTable.at(key).clear();
        mFlowTable.erase(key);
        it = mLastArrived.erase(it); // 更新迭代器
      }
    }
    hd_debug(this->mFlowTable.size());
    hd_debug(this->mSendQueue.size());
  }
  hd_debug(YELLOW("void cleanerJob() 结束"));
}
