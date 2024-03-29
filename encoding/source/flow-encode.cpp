// ReSharper disable CppParameterMayBeConstPtrOrRef
// ReSharper disable CppParameterMayBeConst
#include <iostream>

#include "flow-encode.hpp"
#include "transform.hpp"

#pragma region transform

torch::Tensor transform::z_score_norm(torch::Tensor& data) {
  if (data.dim() == 1) data = data.unsqueeze(1);
  const torch::Tensor mean = nanmean(data, 1, true);
  /// standard deviation; should be torch::nanstd
  torch::Tensor std = nansum(data, 1, true);
  std = where(std == torch::zeros(std.sizes()), torch::tensor(1e-8, data.options()), std);
  torch::Tensor normalized_data = nan_to_num((data - mean) / std);
  if (data.dim() == 2 and data.size(1) == 1) {
    normalized_data = normalized_data.squeeze(1);
  }
  return normalized_data.to(torch::kFloat32);
}

torch::Tensor transform::convert_to_npy(hd_flow const& flow) {
  std::vector<torch::Tensor> flow_data_list;
  flow_data_list.reserve(flow.count);
  for (auto const& packet : flow._packet_list) {
    int _blob_size = static_cast<int>(packet.mBlobData.size());
    const auto _blob_data = (void*)packet.mBlobData.data();
    // _ft: (128+p)
    auto _ft = torch::from_blob(_blob_data, {_blob_size}, torch::kU8);
    if (flow.protocol == IPPROTO_TCP) {
      flow_data_list.emplace_back(torch::concat({
        _ft.slice(0, 0, 12),
        _ft.slice(0, 20, 60),
        _ft.slice(0, 64)
      }));
    } else {
      flow_data_list.emplace_back(torch::concat({
        _ft.slice(0, 0, 12),
        _ft.slice(0, 20, 120),
        _ft.slice(0, 124)
      }));
    }
  }
  // shape: (num_pkt, pkt_len) 116+payload 10,148
  return stack(flow_data_list, 0).to(torch::kU8);
}

/// @brief 构建滑动窗口
/// @param flow_tensors Tensor in shape of (flow_count, (flow_shape)), <br>
/// in which <code>flow_shape</code> is supposed to be (packet_count, packet_length)
/// @param width slide window stride/width
/// @param pkt_actual_len  I don't know neither
/// @return tuple of 2 tensors correspond to <code>window_arr</code> and <code>flow_index_arr</code>
std::tuple<torch::Tensor, std::vector<std::pair<int, int>>>
transform::build_slide_window(std::vector<torch::Tensor> const& flow_tensors, int width, int pkt_actual_len) {
  std::vector<torch::Tensor> sw_list;
  std::vector<std::pair<int, int>> sw_indices;
  sw_indices.reserve(flow_tensors.size());
  for (auto const& flow : flow_tensors) {
    const auto num_packet = flow.size(0);
    // 包个数和滑动窗口长度关系判断,可以被优化掉
    // if (num_packet < width) [[unlikely]] continue;
    sw_list.reserve(num_packet - width + 1);
    int sw_start = static_cast<int>(sw_list.size());// slide_window start
    for (int left = 0, right = left + width; right <= num_packet; ++left, ++right) {
      sw_list.emplace_back(flow.slice(0, left, right).flatten());
    }
    int sw_end = static_cast<int>(sw_list.size());// slide_window end
    // if (window_end_index != window_start_index) [[likely]] {
    sw_indices.emplace_back(sw_start, sw_end);
    // }
  }
  auto sw_tensors = torch::stack(sw_list, 0).to(torch::kFloat32);
  sw_tensors = z_score_norm(sw_tensors);
  sw_tensors = sw_tensors.slice(1, 0, sw_tensors.size(1) - pkt_actual_len);
  return std::make_tuple(sw_tensors, sw_indices);
}

torch::Tensor
transform::merge_flow(torch::Tensor const& predict_flows, std::vector<std::pair<int, int>> const& slide_windows) {
  std::vector<torch::Tensor> temp_tensors;
  temp_tensors.reserve(slide_windows.size());
  torch::nn::AdaptiveMaxPool1d _max_pool1d(torch::nn::AdaptiveMaxPool1dOptions(1));
  for (auto [start, end] : slide_windows) {
    auto merged_flow = predict_flows.slice(0, start, end).transpose(0, 1).unsqueeze(0);
    temp_tensors.emplace_back(_max_pool1d(merged_flow).squeeze());
  }
  return torch::stack(temp_tensors, 0);
}

#pragma endregion transform

#pragma region Exported API

[[maybe_unused]]
// TODO: msg有多条 msg[]
torch::Tensor encode(const std::vector<hd::type::hd_flow>& msg_list) {
  std::string file_path = "./";
  auto [
    flow_encode_model,
    origin_packet_length,
    win_size,
    batch_size,
    calc_device
  ] = load_model_config(file_path);
  std::vector<torch::Tensor> flow_data;
  for (const auto& msg : msg_list) {
    flow_data.push_back(transform::convert_to_npy(msg));
  }
  /// flow_data: in shape of (num_flows, num_flow_packets, packet_length)
  auto [slide_windows, flow_index_arr] = transform::build_slide_window(flow_data, win_size, 136);
  const auto encoded_flows = batch_model_encode(flow_encode_model, slide_windows, batch_size);
  return transform::merge_flow(encoded_flows, flow_index_arr).detach().cpu();
}

[[maybe_unused]] std::tuple<torch::jit::script::Module, int, int, int, torch::Device>
load_model_config(std::string& model_file_path) {
  constexpr int batchSize = 8192;
  constexpr int window_width = 5;
  constexpr int payloadSize = 20;
  model_file_path.assign("/data/Projects/encoding-test/encoder_a128_p20_w5_notime_noip_noport.ptc");
  int _packet_length = 128 + payloadSize;
  torch::jit::script::Module _encode_model;
  try {
    _encode_model = torch::jit::load(model_file_path);
    _encode_model.to(torch::kCPU);
    _encode_model.eval();
  } catch (const torch::Error& e) {
    std::cerr << "模型加载错误: " << e.msg() << std::endl;
    exit(1);
  }
  return std::make_tuple(std::move(_encode_model), _packet_length, window_width, batchSize, torch::kCPU);
}

void print_shape(torch::Tensor const& tensor) {
  const auto sizes = tensor.sizes();
  std::cout << "Tensor sizes: [";
  for (size_t i = 0; i < sizes.size(); ++i) {
    std::cout << sizes[i];
    if (i < sizes.size() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << "]" << std::endl;
}

[[maybe_unused]] torch::Tensor
batch_model_encode(torch::jit::Module& model, const torch::Tensor& data,
                   int64_t batch_size, int64_t max_batch, bool retain) {
  model.eval();

  const int64_t data_length = data.size(0);
  if (data_length <= batch_size) {
    auto const output = model.forward({data}).toTuple();
    // auto confidence = output->elements()[0].toTensor();
    auto flow_hidden = output->elements()[1].toTensor();
    return retain ? flow_hidden : flow_hidden.cpu();
  }
  int64_t batch_count = 0;
  std::vector<torch::Tensor> results_cpu, results;
  results.reserve(data_length / batch_size + 1);
  results_cpu.reserve(data_length / batch_size + 1);
  for (int64_t start = 0; start < data_length; start += batch_size) {
    auto end = std::min(start + batch_size, data_length);
    auto batch_data = data.slice(0, start, end);
    results.emplace_back(model.forward({batch_data}).toTensor().detach());
    batch_count++;
    // ↓↓↓ 把数据转移至CPU， 这在纯CPU的实现中是不必要的
    if (retain or batch_count <= max_batch) continue;
    batch_count = 0;
    results_cpu.emplace_back(concat(results, 0).cpu());
    results.clear();
  }
  if (retain) return concat(results, 0);
  if (batch_count > 0) results_cpu.emplace_back(concat(results, 0).cpu());
  if (results_cpu.size() == 1) return results_cpu.at(0);
  return concat(results_cpu, 0);
}

#pragma endregion Exported API
