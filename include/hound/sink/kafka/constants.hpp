//
// Created by brian on 3/5/24.
//

//
// Created by xhl on 6/20/23.
//

#ifndef HOUND_CONSTANTS_HPP
#define HOUND_CONSTANTS_HPP

#include <string>

namespace hd::keys {
/// kafka 设置
inline constexpr std::string_view KAFKA_BROKERS{"kafka.brokers"};
inline constexpr std::string_view KAFKA_TOPICS{"kafka.topic"};
inline constexpr std::string_view KAFKA_PARTITION{"kafka.partition"};
inline constexpr std::string_view POOL_INIT_SIZE{"kafka.pool.init-size"};
inline constexpr std::string_view POOL_MAX_SIZE{"kafka.pool.maxSize"};
inline constexpr std::string_view CONN_MAX_IDLE_S{"kafka.conn.maxIdleSeconds"};
inline constexpr std::string_view CONN_TIMEOUT_MS{"kafka.conn.timeoutSeconds"};
} // namespace xhl::keys

#endif // HOUND_CONSTANTS_HPP

