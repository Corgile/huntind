//
// hound-torch / model_pool.hpp
// Created by brian on 2024 Apr 05.
//

#ifndef SCOPE_GUARD_HPP
#define SCOPE_GUARD_HPP

#include <functional>

struct scope_guard {
  std::function<void()> collect;

  template <typename Func, typename ...Args> requires std::invocable<Func, std::unwrap_reference_t<Args> ...>
  scope_guard(Func&& func, Args&& ...args) : collect{
    [func = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable {
      std::invoke(std::forward<std::decay_t<Func>>(func), std::unwrap_reference_t<Args>(std::forward<Args>(args)) ...);
    }
  } {}

  ~scope_guard() { collect(); }
  scope_guard(const scope_guard&) = delete;
  scope_guard& operator=(const scope_guard&) = delete;
};

#endif //SCOPE_GUARD_HPP
