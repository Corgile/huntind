//
// hound-torch / connection_pool.hpp. 
// Created by brian on 2024-03-27.
//

#ifndef CONNECTION_POOL_HPP
#define CONNECTION_POOL_HPP

#include <concepts>
#include <functional>
#include <memory>

namespace hd::type {
namespace _internal {
template <typename T>
concept ConnectionType = requires(T a, void* payload, size_t payload_size, const std::string& ordered_key)
{
  { a.pushMessage(payload, payload_size, ordered_key) } -> std::same_as<int>;
  { a.setInUse(true) };
  { a.resetIdleTime() };
  { a.isRedundant() } -> std::same_as<bool>;
};
template <typename T>
concept CreatorType = requires(T a)
{
  { a.operator()() };
  { a.operator()(true) };
};
}

template <_internal::ConnectionType Connection, _internal::CreatorType Creator>
class ConnectionPool {
public:
  ConnectionPool(const size_t maxSize)
    : _maxSize(maxSize), _create(Creator{}) {
    for (int i = 0; i < maxSize; ++i) {
      auto newConn = std::unique_ptr<Connection>(_create());
      auto* rawPtr = newConn.get();
      _pool.push_back(std::move(newConn));
    }
  }

  std::unique_ptr<Connection, std::function<void(Connection*)>> acquire() {
    std::lock_guard lock(_mutex);
    for (auto& conn : _pool) {
      if (conn->isInUse()) continue;
      conn->setInUse(true);
      return {conn.get(), [this](Connection* ptr) { this->release(ptr); }};
    }
    ELOG_WARN << YELLOW("无空闲连接， 创建...");
    auto newConn = std::unique_ptr<Connection>(_create(true));
    auto* rawPtr = newConn.get();
    _pool.push_back(std::move(newConn));
    return {rawPtr, [this](Connection* ptr) { this->release(ptr); }};
  }

  void release(Connection* conn) {
    std::lock_guard lock(_mutex);
    conn->resetIdleTime();
    conn->setInUse(false);
    _pool.emplace_back(conn);
  }

private:
  std::vector<std::unique_ptr<Connection>> _pool;
  std::mutex _mutex;
  size_t _maxSize;
  Creator _create;
};
} // namespace hd::type

#endif //CONNECTION_POOL_HPP
