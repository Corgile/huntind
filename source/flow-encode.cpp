// ReSharper disable CppParameterMayBeConstPtrOrRef
// ReSharper disable CppParameterMayBeConst
#include <iostream>

#include "hound/encode/flow-encode.hpp"

#include <hound/common/macro.hpp>
#include <ylt/easylog.hpp>

#include "hound/encode/transform.hpp"

#pragma region transform

torch::Tensor transform::z_score_norm(torch::Tensor& data) {
  if (data.dim() == 1) data = data.unsqueeze(1).to(hd::global::calc_device);
  const torch::Tensor mean = nanmean(data, 1, true).to(hd::global::calc_device);
  /// standard deviation; should be torch::nanstd
  torch::Tensor std = nansum(data, 1, true).to(hd::global::calc_device);
  auto condition = std == (torch::zeros(std.sizes())).to(hd::global::calc_device);
  auto replace = torch::tensor(1e-8, data.options()).to(hd::global::calc_device);
  std = where(condition, replace, std);
  torch::Tensor normalized_data = nan_to_num((data - mean) / std).to(hd::global::calc_device);
  if (data.dim() == 2 and data.size(1) == 1) {
    normalized_data = normalized_data.squeeze(1);
  }
  return normalized_data.to(torch::kFloat32).to(hd::global::calc_device);
}

torch::Tensor transform::ConvertToTensor(hd_flow const& flow) {
  std::vector<torch::Tensor> flow_data_list;
  flow_data_list.reserve(flow.count);
  for (auto const& packet : flow._packet_list) {
    int _blob_size = static_cast<int>(packet.mBlobData.size());
    auto _blob_data = (void*) packet.mBlobData.data();
    auto _ft = torch::from_blob(_blob_data, {_blob_size}, torch::kU8).to(hd::global::calc_device);
    size_t _end = flow.protocol == IPPROTO_TCP ? 60 : 120;
    size_t _start = flow.protocol == IPPROTO_TCP ? 64 : 124;
    // 使用参数化的方式构建张量
    flow_data_list.emplace_back(
      torch::concat({
                      _ft.slice(0, 0, 12),
                      _ft.slice(0, 20, _end),
                      _ft.slice(0, _start)
                    }));
  }
  // shape: (num_pkt, pkt_len) 116+payload 10,148
  return stack(flow_data_list, 0).to(torch::kU8).to(hd::global::calc_device);
}

/// @brief 构建滑动窗口
/// @param flow_tensors Tensor in shape of (flow_count, (flow_shape)), <br>
/// in which <code>flow_shape</code> is supposed to be (packet_count, packet_length)
/// @param width slide window stride/width
/// @return tuple of 2 tensors correspond to <code>window_arr</code> and <code>flow_index_arr</code>
std::tuple<torch::Tensor, std::vector<std::pair<int, int>>>
transform::BuildSlideWindow(std::vector<torch::Tensor> const& flow_tensors, int width) {
  std::vector<torch::Tensor> sw_list;
  std::vector<std::pair<int, int>> sw_indices;
  sw_indices.reserve(flow_tensors.size());
  for (auto const& flow : flow_tensors) {
    const auto num_packet = flow.size(0);
    sw_list.reserve(num_packet - width + 1);
    int sw_start = static_cast<int>(sw_list.size());// slide_window start
    for (int left = 0, right = left + width; right <= num_packet; ++left, ++right) {
      // todo
      sw_list.emplace_back(flow.slice(0, left, right - 1).flatten());
    }
    int sw_end = static_cast<int>(sw_list.size());// slide_window end
    sw_indices.emplace_back(sw_start, sw_end);
  }
  auto sw_tensors = torch::stack(sw_list, 0).to(torch::kFloat32).to(hd::global::calc_device);
  sw_tensors = z_score_norm(sw_tensors);
  return std::make_tuple(sw_tensors, sw_indices);
}

torch::Tensor
transform::MergeFlow(torch::Tensor const& predict_flows, std::vector<std::pair<int, int>> const& slide_windows) {
  std::vector<torch::Tensor> temp_tensors;
  temp_tensors.reserve(slide_windows.size());
  torch::nn::AdaptiveMaxPool1d _max_pool1d(torch::nn::AdaptiveMaxPool1dOptions(1));
  _max_pool1d->to(hd::global::calc_device);
  for (auto [start, end] : slide_windows) {
    auto merged_flow = predict_flows.slice(0, start, end).transpose(0, 1).unsqueeze(0);
    temp_tensors.emplace_back(_max_pool1d(merged_flow).squeeze());
  }
  return torch::stack(temp_tensors, 0).detach();
}

#pragma endregion transform

#pragma region Exported API

[[maybe_unused]]
torch::jit::script::Module load_model(std::string model_file_path) {
  ELOG_INFO << YELLOW("Worker[") << std::this_thread::get_id() << YELLOW("] 加载模型....");
  model_file_path.assign("models/encoder_a128_p20_w5_notime_noip_noport.ptc");
  torch::jit::script::Module _encode_model;
  try {
    _encode_model = torch::jit::load(model_file_path);
    _encode_model.to(torch::kCPU);
    _encode_model.eval();
    ELOG_INFO << GREEN("加载模型完成");
  } catch (const torch::Error& e) {
    ELOG_INFO << RED("加载模型错误: ") << e.msg();
    exit(1);
  }
  return _encode_model;
}

void print_shape(torch::Tensor const& tensor) {
  std::cout << "Tensor shape: [";
  std::ranges::for_each(tensor.sizes().vec(), [](auto const& item) {
    std::cout << item << ',';
  });
  std::cout << "\x1b[1D]\n";
}

[[maybe_unused]] torch::Tensor
BatchEncode(torch::jit::Module* model, const torch::Tensor& data,
            int64_t batch_size, int64_t max_batch, bool retain) {
  model->to(hd::global::calc_device);
  model->eval();
  const int64_t data_length = data.size(0);
  if (data_length <= batch_size) {
    auto const output = model->forward({data}).toTuple();
    auto flow_hidden = output->elements()[1].toTensor().to(hd::global::calc_device);
    return retain ? flow_hidden : flow_hidden.cpu();
  }
  int64_t batch_count = 0;
  std::vector<torch::Tensor> results_cpu, results;
  results.reserve(data_length / batch_size + 1);
  results_cpu.reserve(data_length / batch_size + 1);
  for (int64_t start = 0; start < data_length; start += batch_size) {
    auto end = std::min(start + batch_size, data_length);
    auto batch_data = data.slice(0, start, end);
    const auto output = model->forward({batch_data}).toTuple();
    results.emplace_back(output->elements()[1].toTensor().to(hd::global::calc_device));
    batch_count++;
    // ↓↓↓ 把数据转移至CPU， 这在纯CPU的实现中是不必要的
    if (retain or batch_count <= max_batch) continue;
    batch_count = 0;
    results_cpu.emplace_back(concat(results, 0).cpu());
    results.clear();
  }
  if (retain) return concat(results, 0).to(hd::global::calc_device);
  if (batch_count > 0) results_cpu.emplace_back(concat(results, 0).cpu());
  if (results_cpu.size() == 1) return results_cpu.at(0);
  return concat(results_cpu, 0);
}

#pragma endregion Exported API
