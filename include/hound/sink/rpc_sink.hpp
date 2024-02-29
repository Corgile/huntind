//
// Created by brian on 11/22/23.
//

#ifndef HOUND_CORO_RPC_HPP
#define HOUND_CORO_RPC_HPP

#include <mutex>
#include <hound/common/core.hpp>
#include <hound/common/flow_check.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

namespace hd::type {
using namespace hd::entity;
using namespace hd::global;

using packet_list = std::vector<hd_packet>;

class RpcSink final {
public:
  explicit RpcSink();

  ~RpcSink();

  void consumeData(ParsedData const& data);

  static std::string serialize(const hd_flow& flow);

private:
  /// \brief 将mFlowTable里面超过timeout但是数量不足的flow删掉
  void cleanerJob();

  void sendTheRest();

private:
  std::mutex accessToFlowTable;
  std::unordered_map<std::string, packet_list> mFlowTable;
  std::unique_ptr<coro_rpc::coro_rpc_server> mInternalRpcServer;

  std::atomic_bool mIsRunning{true};
};
} // entity

#endif //HOUND_CORO_RPC_HPP
