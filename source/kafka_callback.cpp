//
// Created by brian on 3/13/24.
//
#include "hound/sink/kafka/callback.hpp"
#include "hound/common/global.hpp"
#include "hound/common/macro.hpp"

int32_t MyPartitionCB::partitioner_cb(RdKafka::Topic const* topic,
                                      std::string const* key,
                                      int32_t partition_cnt,
                                      void* msg_opaque) {
  const int32_t partition = last_partition++ % partition_cnt;
  last_partition = partition;
  ELOG_DEBUG << "分区: " << partition;
  return partition;
}

void MyReportCB::dr_cb(RdKafka::Message& message) {
  if (message.err()) {
    ELOG_ERROR << "消息推送失败: " << message.errstr();
  }
  if (not hd::global::opt.verbose) return;
  ELOG_INFO << GREEN(">>>消息推送成功至: ") << message.topic_name() << "["
               << message.partition() << "][" << message.offset() << "]";
}

void MyEventCB::event_cb(RdKafka::Event& event) {
  switch (event.type()) {
  case RdKafka::Event::EVENT_ERROR:
    ELOG_ERROR << RED("错误: ") << event.str();
    break;
  case RdKafka::Event::EVENT_STATS:
    ELOG_TRACE << BLUE("EVENT_STATS: ") << event.str();
    break;
  case RdKafka::Event::EVENT_LOG:
    ELOG_WARN << BLUE("EVENT_LOG: ") << event.fac() << event.str();
    break;
  case RdKafka::Event::EVENT_THROTTLE:
    ELOG_TRACE << RED("EVENT_THROTTLE: ") << event.broker_name() << event.str();
    break;
  }
}
