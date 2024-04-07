//
// hound-torch / model_pool.hpp
// Created by brian on 2024 Apr 05.
//

#ifndef SCOPE_GUARD_HPP
#define SCOPE_GUARD_HPP

#include <functional>

/// @brief 管理资源的scope guard
/// @param Resource 被管理(持有)的资源
/*
template <typename Resource>
struct scope_guard {
public:
  using AcquireFunction = std::function<Resource()>;
  using ReleaseFunction = std::function<void(Resource&)>;

  scope_guard(AcquireFunction acquireFunc, ReleaseFunction releaseFunc)
    : releaseFunc_(std::move(releaseFunc)) {
    resource_ = std::move(acquireFunc());
  }

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
  // AcquireFunction acquireFunc_;
  ReleaseFunction releaseFunc_;
  Resource resource_;

  void release() {
    releaseFunc_(resource_);
  }
};
*/

/*
struct scope_guard {
  template <typename Func, typename ...Args>
    requires std::invocable<Func, std::unwrap_reference_t<Args> ...>
  explicit scope_guard(Func&& func, Args&& ...args) {
    collect = [&] mutable {
      std::invoke(std::forward<std::decay_t<Func>>(func), std::forward<std::unwrap_reference_t<Args>>(args) ...);
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
*/

struct scope_guard {
  std::function<void()>f;
  template<typename Func, typename...Args> requires std::invocable<Func, std::unwrap_reference_t<Args>...>
  scope_guard(Func&& func, Args&&...args) :f{ [func = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable {
    std::invoke(std::forward<std::decay_t<Func>>(func), std::unwrap_reference_t<Args>(std::forward<Args>(args))...);
  } }{}
  ~scope_guard() { f(); }
  scope_guard(const scope_guard&) = delete;
  scope_guard& operator=(const scope_guard&) = delete;
};

#endif //SCOPE_GUARD_HPP
