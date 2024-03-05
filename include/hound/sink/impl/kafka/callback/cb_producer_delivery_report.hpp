//
// Created by xhl on 23-5-11.
//

#ifndef HOUND_PRODUCER_DELIVERY_REPORT_CB_HPP
#define HOUND_PRODUCER_DELIVERY_REPORT_CB_HPP

#include <librdkafka/rdkafkacpp.h>
#include <hound/common/macro.hpp>

// 生产者投递报告回调
class ProducerDeliveryReportCb final : public RdKafka::DeliveryReportCb {
public:
  void dr_cb(RdKafka::Message& message) override {
    // 发送出错的回调
    if (message.err()) {
      hd_line(RED("消息推送失败: "), message.errstr());
    } else {// 发送正常的回调
      if (hd::global::opt.verbose)
        hd_line(GREEN("消息推送成功至: "), message.topic_name(),
                "[", message.partition(), "][", message.offset(), "]");
    }
  }

private:
};

#endif // HOUND_PRODUCER_DELIVERY_REPORT_CB_HPP
