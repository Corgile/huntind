//
// Created by brian on 3/13/24.
//

#ifndef HOUND_TRANSFORM_HPP
#define HOUND_TRANSFORM_HPP

#include <torch/torch.h>
#include <hound/type/hd_flow.hpp>

namespace transform {
using namespace torch;
using namespace hd::type;

/// @brief z score normalization checked
/// @param data input
/// @return normalized data
Tensor z_score_norm(Tensor& data);

Tensor convert_to_npy(hd_flow const& flow);

std::tuple<Tensor, Tensor> build_slide_window(Tensor const& flow_list, int win_stride, int actual_pkt_len);

Tensor merge_flow(Tensor const& predict_flows, at::Tensor const& flow_indices);
}
#endif //HOUND_TRANSFORM_HPP
