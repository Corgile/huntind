//
// hound-torch / model_pool.cpp.
// Created by brian on 2024-03-26.
//
#include <hound/model_pool.hpp>
#include <hound/common/global.hpp>

hd::type::ModelPool::ModelPool(const std::string& model_path) {
  global::opt.num_gpus = torch::cuda::device_count();
  const auto m = torch::jit::load(model_path);
  for (int device = 0; device < global::opt.num_gpus; ++device) {
    auto model = new torch::jit::Module(m);
    model->to(torch::Device(torch::kCUDA, device));
    models.push({model, device});
    model->eval();
  }
}

hd::type::ModelPool::~ModelPool() {
  while (!models.empty()) {
    delete models.front().model;
    models.pop();
  }
}

hd::type::ScopeGuard hd::type::ModelPool::borrowModel() {
  std::unique_lock lock(mtx);
  cond.wait(lock, [this] { return !models.empty(); });
  auto _model_wrapper = std::move(models.front());
  models.pop();
  return ScopeGuard(*this, std::move(_model_wrapper));
}

void hd::type::ModelPool::returnModel(ModelWrapper model) {
  {
    std::lock_guard lock(mtx);
    models.push({model.model, model.device});
    model.model = nullptr;
  }
  cond.notify_one();
}

hd::type::ScopeGuard::ScopeGuard(hd::type::ModelPool& pool, ModelWrapper model)
  : pool(pool), modelWrapper(std::move(model)) {}

hd::type::ScopeGuard::~ScopeGuard() {
  pool.returnModel(std::move(modelWrapper));
}

int hd::type::ScopeGuard::getDeviceId() const {
  return modelWrapper.device;
}

torch::jit::Module* hd::type::ScopeGuard::getModel() const { return modelWrapper.model; }
