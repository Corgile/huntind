//
// Created by brian on 3/13/24.
//

#ifndef HOUND_TRANSFORM_HPP
#define HOUND_TRANSFORM_HPP

#include <torch/torch.h>

namespace transform {

using namespace torch;

Tensor z_score_norm(Tensor& data);

std::vector<Tensor> convert_to_npy(hd::type::hd_flow const& msg, int packet_length);

std::tuple<Tensor, Tensor> build_slide_window(std::vector<Tensor> const& flow_list, int win_size, int actual_pkt_len);

Tensor merge_flow(std::vector<Tensor> const& predict_flows, Tensor const& flow_indices);
}
#endif //HOUND_TRANSFORM_HPP
