//
// Created by xhl on 23-5-11.
//

#ifndef HOUND_PRODUCER_DELIVERY_REPORT_CB_HPP
#define HOUND_PRODUCER_DELIVERY_REPORT_CB_HPP

#include <librdkafka/rdkafkacpp.h>
#include <hound/common/macro.hpp>
#include <hound/common/global.hpp>

// 生产者投递报告回调
class ProducerDeliveryReportCb final : public RdKafka::DeliveryReportCb {
public:
  void dr_cb(RdKafka::Message& message) override;

private:
};

#endif // HOUND_PRODUCER_DELIVERY_REPORT_CB_HPP
