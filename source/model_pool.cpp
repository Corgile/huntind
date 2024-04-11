//
// hound-torch / model_pool.cpp.
// Created by brian on 2024-03-26.
//
#include <hound/model_pool.hpp>
#include <hound/common/global.hpp>

hd::type::ModelPool::ModelPool(int size, const std::string& model_path) {
  auto m = torch::jit::load(model_path);
  for (int i = 0; i < size; ++i) {
    auto model = new torch::jit::Module(m);
    models.push(model);
  }
}

hd::type::ModelPool::~ModelPool() {
  while (!models.empty()) {
    delete models.front();
    models.pop();
  }
}

hd::type::ScopeGuard hd::type::ModelPool::borrowModel() {
  std::unique_lock lock(mtx);
  cond.wait(lock, [this] { return !models.empty(); });
  const auto model = models.front();
  models.pop();
  return ScopeGuard(*this, model);
}

void hd::type::ModelPool::returnModel(torch::jit::Module* model) {
  {
    std::lock_guard lock(mtx);
    models.push(model);
  }
  cond.notify_one();
}

hd::type::ScopeGuard::ScopeGuard(hd::type::ModelPool& pool, torch::jit::Module* model)
  : pool(pool), model(model) {}

hd::type::ScopeGuard::~ScopeGuard() {
  pool.returnModel(model);
}

torch::jit::Module* hd::type::ScopeGuard::get() const { return model; }
