//
// Created by brian on 3/13/24.
//

#ifndef HOUND_TRANSFORM_HPP
#define HOUND_TRANSFORM_HPP

#include "hound/type/hd_flow.hpp"

namespace transform {
using namespace hd::type;

/// @brief 标准化
/// @param data Tensor indicating slide window list
/// @param
/// @return normalized Tensor
torch::Tensor
z_score_norm(torch::Tensor const& data, torch::Device&);

/// @brief 将flow转换成Tensor
/// @param flow \p hd_flow& as input
/// @param option tensor 选项(device, dtype,...)
/// @param device
/// @param width
/// @return Tensor in \code (num_packets, 136)\endcode
torch::Tensor
FlowToTensor(hd_flow& flow, torch::TensorOptions const& option, torch::Device& device, int width);

/// @brief 将packet中的二进制数转换成tensor中的数据
/// @param packet parsed packet (we want its blob data)
/// @param protocol packet transportation protocol (for slicing)
/// @param device
/// @return Tensor in shape of \code (1, 136)\endcode
torch::Tensor
PacketToTensor(parsed_packet const& packet, long protocol, torch::Device& device);

/// @brief 构建滑动窗钩
/// @param flow_list flow list / vector
/// @param width window size
/// @param device cuda设备
/// @return tuple<Tensor, Tensor> (slide_windows, window_indices)
std::tuple<torch::Tensor, torch::Tensor>
BuildSlideWindow(flow_vec_ref const& flow_list, int width, torch::Device& device);

[[deprecated]]
std::pair<torch::Tensor, torch::Tensor>
BuildSlideWindowConcurrently(flow_vec_ref const& flow_list, int width, torch::Device& device);


/// @param predict_flows 输入需要被分类的flow
/// @param index_arr 滑动窗口下标
/// @param device 显卡
/// @return Tensor in shape of (num_flows, flow_hidden_dim), here: (num_flows, 128)
torch::Tensor
MergeFlow(torch::Tensor const& predict_flows, torch::Tensor const& index_arr, torch::Device& device);
}
#endif //HOUND_TRANSFORM_HPP
