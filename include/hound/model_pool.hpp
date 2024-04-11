//
// hound-torch / model_pool.hpp.
// Created by brian on 2024-03-26.
//

#ifndef MODEL_POOL_HPP
#define MODEL_POOL_HPP

#include <queue>
#include <torch/torch.h>
#include <torch/script.h>

// TODO @see purecpp.cn 模板解耦
namespace hd::type {
class ScopeGuard;

class ModelPool {
public:
  ModelPool() = default;
  ModelPool(int size, const std::string& model_path);

  ScopeGuard borrowModel();
  void returnModel(torch::jit::Module* model);

  ModelPool& operator=(ModelPool&& other) noexcept {
    if (this == &other) return *this;
    models.swap(other.models);
    return *this;
  }

  ~ModelPool();

private:
  std::mutex mtx;
  std::condition_variable cond;
  std::queue<torch::jit::Module*> models;
};

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

#endif //MODEL_POOL_HPP
