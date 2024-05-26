#ifndef ENCODER_LIBRARY_HPP
#define ENCODER_LIBRARY_HPP

#include <torch/script.h>

#include "hound/type/hd_flow.hpp"

/**
 *
 * @param model
 * @param data 要编码的数据（flow的 Tensor 表示）
 * @param batch_size batch size
 * @param max_batch 因为显存资源优先， 模型每预测 <code>max_batch</code> 个批次会将GPU上的预测结果合并并转移到内存中去。
 * 如果取值过大, 可能会导致显存溢出; 如果取值过小, 则会造成GPU-CPU的IO瓶颈
 * @param stay_on_gpu true for calculated data should stay on the calculation device
 * @return encoded data on GPU
 */
torch::Tensor
BatchEncode(
  torch::jit::Module* model, const torch::Tensor& data,
  int64_t batch_size, int64_t max_batch, bool stay_on_gpu);

void print_shape(torch::Tensor const& tensor);
void print_tensor(torch::Tensor const& tensor);

#endif // ENCODER_LIBRARY_HPP
