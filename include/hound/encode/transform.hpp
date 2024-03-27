//
// Created by brian on 3/13/24.
//

#ifndef HOUND_TRANSFORM_HPP
#define HOUND_TRANSFORM_HPP

#include <torch/torch.h>
#include "hound/type/hd_flow.hpp"

namespace transform {
using namespace hd::type;

/// @brief z score normalization checked
/// @param data input
/// @return normalized data
torch::Tensor z_score_norm(torch::Tensor& data);

torch::Tensor ConvertToTensor(hd_flow const& flow);

std::tuple<torch::Tensor, std::vector<std::pair<int, int>>>
BuildSlideWindow(std::vector<torch::Tensor> const& flow_tensors, int width = 5);

///
/// @param predict_flows 输入需要被分类的flow
/// @param slide_windows 滑动窗口本口，<code>vector&</code> of <code>pair<start:int, end:int></code>
/// @return Tensor in shape of (num_flows, flow_hidden_dim), here: (20, 128)
torch::Tensor MergeFlow(torch::Tensor const& predict_flows, std::vector<std::pair<int, int>> const& slide_windows);
}
#endif //HOUND_TRANSFORM_HPP
