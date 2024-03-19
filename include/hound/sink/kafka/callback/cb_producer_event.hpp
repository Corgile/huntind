//
// Created by brian on 3/5/24.
//

//
// Created by xhl on 23-5-11.
//

#ifndef HOUND_PRODUCER_EVENT_CB_HPP
#define HOUND_PRODUCER_EVENT_CB_HPP

#include <librdkafka/rdkafkacpp.h>

// 生产者事件回调函数
class ProducerEventCb final : public RdKafka::EventCb {
public:
  void event_cb(RdKafka::Event& event) override;
};

#endif // HOUND_PRODUCER_EVENT_CB_HPP

