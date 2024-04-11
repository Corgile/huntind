// ReSharper disable CppParameterMayBeConstPtrOrRef
// ReSharper disable CppParameterMayBeConst
#include <iostream>

#include "hound/encode/flow-encode.hpp"

#include <hound/common/macro.hpp>
#include <ylt/easylog.hpp>

#include "hound/encode/transform.hpp"

#pragma region transform

torch::Tensor transform::z_score_norm(torch::Tensor const& data) {
  const auto options = torch::TensorOptions().dtype(torch::kF32).device(hd::global::calc_device);
  const torch::Tensor mean = torch::nanmean(data, /*dim=*/1, /*keepdim=*/true);
  torch::Tensor std = torch::std(data, /*dim=*/1, /*unbiased=*/true, /*keepdim=*/true);
  std = torch::where(std == 0, torch::tensor(1e-8, options), std);
  torch::Tensor normalized_data = torch::nan_to_num((data - mean) / std);
  if (data.size(1) == 1) {
    normalized_data = normalized_data.squeeze(1);
  }
  return normalized_data;
}

torch::Tensor transform::FlowToTensor(hd_flow const& flow, torch::TensorOptions const& option) {
  torch::Tensor flow_tensor = torch::empty({flow.count, 136}, option);
  for (size_t packet_idx = 0; packet_idx < flow._packet_list.size(); ++packet_idx) {
    const parsed_packet& packet = flow._packet_list[packet_idx];
    flow_tensor[packet_idx] = PacketToTensor(packet, flow.protocol);
  }
  return flow_tensor;
}

torch::Tensor transform::PacketToTensor(parsed_packet const& packet, long protocol) {
  constexpr int n_digits = 148;
  const auto _blob_data = (void*)packet.mBlobData.data();
  const auto options = torch::TensorOptions().dtype(torch::kU8);
  const auto _ft = torch::from_blob(_blob_data, {n_digits}, options).to(hd::global::calc_device);
  long _end = protocol == IPPROTO_TCP ? 60 : 120;
  long _start = protocol == IPPROTO_TCP ? 64 : 124;
  return torch::concat(
    {
      _ft.slice(0, 0, 12),
      _ft.slice(0, 20, _end),
      _ft.slice(0, _start)
    }, 0).to(hd::global::calc_device);
}

std::tuple<torch::Tensor, torch::Tensor> transform::BuildSlideWindow(flow_vector& flow_list, int width) {
  const auto index_option = torch::TensorOptions().dtype(torch::kInt32).device(hd::global::calc_device);
  const auto window_option = torch::TensorOptions().dtype(torch::kFloat32).device(hd::global::calc_device);

  long num_windows = [width](flow_vector const& vec) {
    long res = 0;
    std::ranges::for_each(vec, [&res, width](hd_flow const& flow) {
      res += flow.count - width + 1;
    });
    return res;
  }(flow_list);
  torch::Tensor index_list = torch::empty({static_cast<int>(flow_list.size()), 2}, index_option);
  torch::Tensor window_list = torch::empty({num_windows, (width - 1) * 136}, window_option);

  for (long offset = 0, flow_idx = 0; flow_idx < flow_list.size(); ++flow_idx) {
    hd_flow const& flow = flow_list[flow_idx];
    const int packet_count = flow._packet_list.size();
    torch::Tensor flow_tensor = FlowToTensor(flow, window_option);
    for (int left = 0, right = width; right <= packet_count; ++left, ++right) {
      window_list[offset + left] = flow_tensor.slice(0, left, right - 1).flatten();
    }
    long _window_count = offset + packet_count - width + 1;
    index_list[flow_idx] = torch::tensor({offset, _window_count}, index_option);
    offset = _window_count;
  }
  window_list = z_score_norm(window_list);
  return std::make_tuple(window_list, index_list);
}

torch::Tensor
transform::MergeFlow(torch::Tensor const& predict_flows, torch::Tensor const& index_arr) {
  auto options = torch::TensorOptions().dtype(torch::kFloat32).device(hd::global::calc_device);
  torch::Tensor temp_tensors = torch::empty({index_arr.size(0), 128}, options);
  const auto temp = index_arr.cpu();
  auto ptr = temp.accessor<int32_t, 2>();
  for (int i = 0; i < temp.size(0); ++i) {
    auto merged_flow = predict_flows.slice(0, ptr[i][0], ptr[i][1]);
    auto [max_values, indices] = torch::max(merged_flow, 0);
    temp_tensors[i] = max_values;
  }
  return temp_tensors;
}

#pragma endregion transform

#pragma region Exported API

void print_shape(torch::Tensor const& tensor) {
  std::cout << "Tensor shape: [";
  std::ranges::for_each(tensor.sizes().vec(), [](auto const& item) {
    std::cout << item << ',';
  });
  std::cout << "\x1b[2D]\n";
}

void print_tensor(torch::Tensor const& tensor) {
  if (tensor.dim() != 2 || tensor.size(1) != 2) {
    std::cerr << "Error: Tensor is not of shape n x 2" << std::endl;
    return;
  }
  auto temp = tensor.cpu();
  auto tensor_accessor = temp.accessor<int32_t, 2>();  // 使用 float 类型和 2D 访问器
  for (int i = 0; i < std::min(10, (int)tensor_accessor.size(0)); ++i) {
    std::cout << "(" << tensor_accessor[i][0] << ", " << tensor_accessor[i][1] << ")\n";
  }
}

torch::Tensor
BatchEncode(const torch::Tensor& data, int64_t batch_size,
            int64_t max_batch, bool stay_on_gpu) {
  const auto modelGuard = hd::global::model_pool.borrowModel();
  const auto model = modelGuard.get();

  const int64_t data_size = data.size(0);
  std::vector<torch::Tensor> CPU_results, GPU_results;
  GPU_results.reserve(data_size / batch_size + 1);
  CPU_results.reserve(data_size / batch_size + 1);

  for (int64_t start = 0; start < data_size; start += batch_size) {
    auto end = std::min(start + batch_size, data_size);
    auto batch_data = data.slice(0, start, end);
    GPU_results.emplace_back(EncodeOneBatch(model, batch_data, stay_on_gpu));
    // if (stay_on_gpu) continue;
    // ↓↓↓ 把数据转移至CPU防止GPU-OOM
    if (GPU_results.size() < max_batch) continue;
    CPU_results.emplace_back(concat(GPU_results, 0).cpu());
    GPU_results.clear();
  }
  // if (stay_on_gpu) return concat(GPU_results, 0);//.to(hd::global::calc_device);
  if (not GPU_results.empty()) {
    CPU_results.emplace_back(concat(GPU_results, 0).cpu());
    GPU_results.clear();
  }
  return concat(CPU_results, 0);
}

torch::Tensor EncodeOneBatch(torch::jit::script::Module* model, const torch::Tensor& data, bool stay_on_gpu) {
  auto const output = model->forward({data}).toTuple();
  auto flow_hidden = output->elements()[1].toTensor();
  return stay_on_gpu ? flow_hidden : flow_hidden.cpu();
}

#pragma endregion Exported API
