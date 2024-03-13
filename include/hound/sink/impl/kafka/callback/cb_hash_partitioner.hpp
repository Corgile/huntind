//
// Created by xhl on 23-5-11.
//

#ifndef HOUND_HASH_PARTITIONER_CB_HPP
#define HOUND_HASH_PARTITIONER_CB_HPP

#include <librdkafka/rdkafkacpp.h>

/// 生产者自定义分区策略回调：partitioner_cb
class HashPartitionerCb : public RdKafka::PartitionerCb {
public:
  /// @brief 返回 topic 中使用 flowId 的分区，msg_opaque 置 NULL
  /// @return 返回分区，(0, partition_cnt)
  int32_t partitioner_cb(const RdKafka::Topic*, const std::string*, int32_t, void*) override;

private:
  /// 自定义哈希函数
  inline int32_t generate_hash(const char* str, const size_t len);
};

#endif // HOUND_HASH_PARTITIONER_CB_HPP
