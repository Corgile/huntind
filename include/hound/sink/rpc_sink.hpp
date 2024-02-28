// //
// // Created by brian on 11/22/23.
// //
//
// #ifndef HOUND_CORO_RPC_HPP
// #define HOUND_CORO_RPC_HPP
//
// #include <condition_variable>
// #include <mutex>
// #include <thread>
// #include <hound/common/core.hpp>
// #include <hound/common/flow_check.hpp>
//
// namespace hd::type {
// using namespace hd::entity;
// using namespace hd::global;
//
// using packet_list = std::vector<hd_packet>;
//
// class RpcSink final {
// public:
//   explicit RpcSink();
//
//   ~RpcSink();
//
//   void consumeData(ParsedData const& data);
//
// private:
//   void sendingJob();
//
//   /// \brief 将mFlowTable里面超过timeout但是数量不足的flow删掉
//   void cleanerJob();
//
//   void sendTheRest();
//
// private:
//   std::mutex mtxAccessToFlowTable;
//   std::unordered_map<std::string, packet_list> mFlowTable;
//   std::mutex mtxAccessToLastArrived;
//   std::unordered_map<std::string, long> mLastArrived;
//
//   std::mutex mtxAccessToQueue;
//   std::queue<hd_flow> mSendQueue;
//   std::condition_variable cvMsgSender{};
//
//   std::atomic_bool mIsRunning{true};
// };
// } // entity
//
// #endif //HOUND_CORO_RPC_HPP
