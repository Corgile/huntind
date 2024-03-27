//
// hound-torch / scope_guard.hpp. 
// Created by brian on 2024-03-26.
//

#ifndef SCOPE_GUARD_HPP
#define SCOPE_GUARD_HPP

#include <queue>
#include <torch/script.h>

namespace hd :: type {
class ScopeGuard;

class ModelPool {
public:
  ModelPool(int size, const std::string& model_path);

  ~ModelPool();

  ScopeGuard borrowModel();

  void returnModel(torch::jit::Module* model);

private:
  std::mutex mtx;
  std::condition_variable cond;
  std::queue<torch::jit::Module*> models;
};

// ScopeGuard for managing the lifecycle of borrowed models.
class ScopeGuard {
public:
  ScopeGuard(hd::type::ModelPool& pool, torch::jit::Module* model);

  ~ScopeGuard();

  torch::jit::Module* get() const;

private:
  hd::type::ModelPool& pool;
  torch::jit::Module* model;
};
} // type

#endif //SCOPE_GUARD_HPP
