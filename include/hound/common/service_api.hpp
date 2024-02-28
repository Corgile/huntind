//
// hound / service_api.hpp. 
// Created by brian on 2024-02-27.
//

#ifndef SERVICE_API_HPP
#define SERVICE_API_HPP

#include <dbg.h>
#include <string>
#include <ylt/struct_json/json_writer.h>
#include <hound/sink/rpc_sink.hpp>

namespace service {
inline std::queue<hd::entity::hd_flow> rpc_msg_queue;
inline std::mutex mtx_queue_access;

inline std::string sendingJob() {
  std::unique_lock lock(mtx_queue_access);
  auto front{rpc_msg_queue.front()};
  rpc_msg_queue.pop();
  lock.unlock();
  return hd::type::RpcSink::serialize(front);
}
}
#endif //SERVICE_API_HPP
