//
// Created by xhl on 23-5-11.
//

#ifndef HOUND_HASH_PARTITIONER_CB_HPP
#define HOUND_HASH_PARTITIONER_CB_HPP

#include <rdkafkacpp.h>

// 生产者自定义分区策略回调：partitioner_cb
class HashPartitionerCb : public RdKafka::PartitionerCb {
public:
  // @brief 返回 topic 中使用 flowId 的分区，msg_opaque 置 NULL
  // @return 返回分区，(0, partition_cnt)
  int32_t partitioner_cb(const RdKafka::Topic* topic, const std::string* key,
                         int32_t partition_cnt, void* msg_opaque) override {
    char msg[128] = {0};
    // 用于自定义分区策略：这里用 hash。例：轮询方式：p_id++ % partition_cnt
    int32_t const partition_id =
      static_cast<int32_t>(generate_hash(key->c_str(), key->size()) % partition_cnt);
    sprintf(msg,
            "HashPartitionerCb:topic:[%s], flowId:[%s], partition_cnt:[%d], "
            "partition_id:[%d]",
            topic->name().c_str(), key->c_str(), partition_cnt, partition_id);
    return partition_id;
  }

private:
  // 自定义哈希函数
  static unsigned int generate_hash(const char* str, const size_t len) {
    unsigned int hash = 5381;
    for (size_t i = 0; i < len; i++) {
      hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
  }
};

#endif // HOUND_HASH_PARTITIONER_CB_HPP
