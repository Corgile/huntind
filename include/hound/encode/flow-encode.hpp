#ifndef ENCODER_LIBRARY_HPP
#define ENCODER_LIBRARY_HPP

#include <torch/script.h>

#include "hound/type/hd_flow.hpp"

/**
 * 
 * @param model 编码模型，这里就是那个<code>ContextBuilder</code>
 * @param data 要编码的数据（flow的 Tensor 表示）
 * @param batch_size batch size
 * @param max_batch 因为显存资源优先， 模型每预测 <code>max_batch</code> 个批次会将GPU上的预测结果合并并转移到内存中去。
 * 如果取值过大, 可能会导致显存溢出; 如果取值过小, 则会造成GPU-CPU的IO瓶颈
 * @param retain true for calculated data should stay on the calculation device
 * @return encoded data
 */
[[maybe_unused]] torch::Tensor
BatchEncode(torch::jit::script::Module* model, const torch::Tensor& data, int64_t batch_size,
                   int64_t max_batch = 20, bool retain = false);

// 用于加载模型和配置的函数
[[maybe_unused]] torch::jit::script::Module
load_model(std::string model_file_path = "./");

void print_shape(torch::Tensor const& tensor);

#endif // ENCODER_LIBRARY_HPP
