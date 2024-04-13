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

struct ModelWrapper {
  ModelWrapper(torch::jit::Module* model, int device)
    : model(model),
      device(device) {}

  ModelWrapper() = default;

  ModelWrapper(ModelWrapper&& other) noexcept
    : model{other.model},
      device{other.device} {
    other.model = nullptr;
  }

  ModelWrapper& operator=(ModelWrapper&& other) noexcept {
    if (this == &other) return *this;
    model = other.model;
    device = other.device;
    other.model = nullptr;
    return *this;
  }

  torch::jit::Module* model;
  int device;
};

class ModelPool {
public:
  ModelPool() = default;
  ModelPool(const std::string& model_path);

  ScopeGuard borrowModel();
  void returnModel(ModelWrapper model);

  ModelPool& operator=(ModelPool&& other) noexcept {
    if (this == &other) return *this;
    models.swap(other.models);
    return *this;
  }

  ~ModelPool();

private:
  std::mutex mtx;
  std::condition_variable cond;
  std::queue<ModelWrapper> models;
};

class ScopeGuard {
public:
  ScopeGuard(hd::type::ModelPool& pool, ModelWrapper model);

  ~ScopeGuard();

  torch::jit::Module* getModel() const;

  int getDeviceId() const;

private:
  hd::type::ModelPool& pool;
  ModelWrapper modelWrapper;
};
} // type

#endif //MODEL_POOL_HPP
