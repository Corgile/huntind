//
// hound-torch / model_pool.hpp
// Created by brian on 2024 Apr 05.
//

#ifndef SCOPE_GUARD_HPP
#define SCOPE_GUARD_HPP

#include <functional>

/// @brief 管理资源的scope guard
/// @tparam Resource 被管理(持有)的资源
template <typename Resource>
struct scope_guard {
public:
  using AcquireFunction = std::function<Resource()>;
  using ReleaseFunction = std::function<void(Resource)>;

  scope_guard(AcquireFunction acquireFunc, ReleaseFunction releaseFunc)
    : acquireFunc_(acquireFunc), releaseFunc_(releaseFunc), resource_(acquireFunc_()) {}

  ~scope_guard() {
    release();
  }

  /// TODO: 不应该让 scope guard 管理资源
  [[nodiscard]] Resource& resource() {
    return resource_;
  }

public:
  scope_guard(const scope_guard&) = delete;
  scope_guard(scope_guard&& other) = delete;
  scope_guard& operator=(const scope_guard&) = delete;
  scope_guard& operator=(scope_guard&& other) = delete;

private:
  AcquireFunction acquireFunc_;
  ReleaseFunction releaseFunc_;
  Resource resource_;

  void release() {
    releaseFunc_(std::move(resource_));
  }
};

/// @brief 不管理资源的scope guard
template <>
struct scope_guard<void> {
  template <typename Func, typename ...Args>
    requires std::invocable<Func, std::unwrap_reference_t<Args> ...>
  explicit scope_guard(Func&& func, Args&& ...args) {
    collect = [func = std::decay_t<Func>(func), ...args = std::unwrap_reference_t<Args>(args)] mutable {
      std::invoke(std::forward<Func>(func), std::forward<Args>(args) ...);
    };
  }

  ~scope_guard() { collect(); }

public:
  scope_guard(const scope_guard&) = delete;
  scope_guard(scope_guard&& other) = delete;
  scope_guard& operator=(const scope_guard&) = delete;
  scope_guard& operator=(scope_guard&& other) = delete;

private:
  std::function<void()> collect;
};

#endif //SCOPE_GUARD_HPP
