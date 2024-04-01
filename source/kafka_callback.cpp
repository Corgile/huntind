//
// Created by brian on 3/13/24.
//
#include "hound/sink/kafka/callback/cb_hash_partitioner.hpp"
#include "hound/sink/kafka/callback/cb_producer_delivery_report.hpp"
#include "hound/sink/kafka/callback/cb_producer_event.hpp"

#include "hound/common/global.hpp"
#include "hound/common/macro.hpp"

int32_t HashPartitionerCb::partitioner_cb(RdKafka::Topic const *topic,
                                          std::string const *key,
                                          int32_t partition_cnt,
                                          void *msg_opaque) {
  char msg[128]{};
  // 用于自定义分区策略：这里用 hash。例：轮询方式：p_id++ % partition_cnt
  auto partition{generate_hash(key->c_str(), key->size()) % partition_cnt};
  sprintf(msg, "topic:[%s], flowId:[%s], partition_cnt:[%d], partition_id:[%d]",
          topic->name().c_str(), key->c_str(), partition_cnt, partition);
  return partition;
}

int32_t HashPartitionerCb::generate_hash(char const *str, size_t const len) {
  int32_t hash = 5381;
  for (size_t i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + str[i];
  }
  return hash;
}

void ProducerDeliveryReportCb::dr_cb(RdKafka::Message &message) {
  if (not hd::global::opt.verbose)
    return;
  // 发送出错的回调
  if (message.err()) {
    ELOG_ERROR << "消息推送失败: " << message.errstr();
  } else {
    ELOG_TRACE << GREEN("消息推送成功至: ") << message.topic_name() << "["
               << message.partition() << "][" << message.offset() << "]";
  }
}

void ProducerEventCb::event_cb(RdKafka::Event &event) {
  switch (event.type()) {
  case RdKafka::Event::EVENT_ERROR:
    ELOG_ERROR << RED("错误: ") << event.str();
    break;
  case RdKafka::Event::EVENT_STATS:
    ELOG_TRACE << BLUE("EVENT_STATS: ") << event.str();
    break;
  case RdKafka::Event::EVENT_LOG:
    ELOG_TRACE << BLUE("EVENT_LOG: ") << event.fac();
    break;
  case RdKafka::Event::EVENT_THROTTLE:
    ELOG_TRACE << RED("EVENT_THROTTLE: ") << event.broker_name();
    break;
  }
}
