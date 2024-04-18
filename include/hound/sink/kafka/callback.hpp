//
// hound-torch / callback.hpp
// Created by brian on 2024 Apr 05.
//

#ifndef KAFKA_CALLBACK_HPP
#define KAFKA_CALLBACK_HPP
#include <librdkafka/rdkafkacpp.h>

// 生产者投递报告回调
class MyReportCB final : public RdKafka::DeliveryReportCb {
public:
  MyReportCB();
  void dr_cb(RdKafka::Message& message) override;
};

// 生产者事件回调函数
class MyEventCB final : public RdKafka::EventCb {
public:
  MyEventCB();
  void event_cb(RdKafka::Event& event) override;
};

/// 生产者自定义分区策略回调：partitioner_cb
class MyPartitionCB final : public RdKafka::PartitionerCb {
public:
  MyPartitionCB();
  mutable int32_t last_partition = 0;
  /// @brief 返回 topic 中使用 flowId 的分区，msg_opaque 置 NULL
  /// @return 返回分区，(0, partition_cnt)
  int32_t partitioner_cb(const RdKafka::Topic*, const std::string*, int32_t, void*) override;
};

#endif //KAFKA_CALLBACK_HPP
