#ifndef ENCODER_LIBRARY_HPP
#define ENCODER_LIBRARY_HPP

#include <torch/torch.h>
#include <torch/script.h>

#include <hound/type/hd_flow.hpp>
#include <hound/encoding/detail/transform.hpp>

[[maybe_unused]]
torch::Tensor encode(const hd::type::hd_flow& msg);

[[maybe_unused]] torch::Tensor
batch_model_encode(torch::jit::script::Module& model, torch::Tensor data, int64_t batch_size,
                   int64_t max_num_batches = 20, bool retain = false);

// 用于加载模型和配置的函数
[[maybe_unused]] std::tuple<torch::jit::script::Module, int, int, int, torch::Device>
load_model_config(std::string& encodeModelPath);

#endif // ENCODER_LIBRARY_HPP
