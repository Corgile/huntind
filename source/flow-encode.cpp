// ReSharper disable CppParameterMayBeConstPtrOrRef
// ReSharper disable CppParameterMayBeConst
#include <future>
#include <iostream>

#include <hound/common/macro.hpp>
#include <hound/common/timer.hpp>

#include <hound/encode/flow-encode.hpp>
#include <hound/encode/transform.hpp>

#pragma region transform

torch::Tensor transform::z_score_norm(torch::Tensor const& data, torch::Device& device) {
  const auto options = torch::TensorOptions().dtype(torch::kF32).device(device);
  const torch::Tensor mean = torch::nanmean(data, /*dim=*/1, /*keepdim=*/true);
  torch::Tensor std = torch::std(data, /*dim=*/1, /*unbiased=*/true, /*keepdim=*/true);
  std = torch::where(std == 0, torch::tensor(1e-8, options), std);
  torch::Tensor normalized_data = torch::nan_to_num((data - mean) / std);
  if (data.size(1) == 1) {
    normalized_data = normalized_data.squeeze(1);
  }
  return normalized_data;
}

torch::Tensor
transform::FlowToTensor(hd_flow& flow, torch::TensorOptions const& option, torch::Device& device, int width) {
  const auto w = flow.count - flow.count % width;
  torch::Tensor flow_tensor = torch::empty({w, 136}, option);
  for (size_t _idx = 0; _idx < w; ++_idx) {
    flow_tensor[_idx] = PacketToTensor(flow.at(_idx), flow.protocol, device);
  }
  return flow_tensor;
}

torch::Tensor
transform::PacketToTensor(parsed_packet const& packet, long protocol, torch::Device& device) {
  constexpr int n_digits = 148;
  const auto _blob_data = (void*)packet.mBlobData.data();
  const auto _ft = torch::from_blob(_blob_data, {n_digits}, torch::kU8).to(device);
  long _end = protocol == IPPROTO_TCP ? 60 : 120;
  long _start = protocol == IPPROTO_TCP ? 64 : 124;
  return torch::concat(
    {
      _ft.slice(0, 0, 12),
      _ft.slice(0, 20, _end),
      _ft.slice(0, _start)
    }, 0);
}

std::pair<torch::Tensor, torch::Tensor>
transform::BuildSlideWindowConcurrently(flow_vec_ref const& flow_list, int width, torch::Device& device) {
  const auto index_option = torch::TensorOptions().dtype(torch::kI32).device(device);
  const auto window_option = torch::TensorOptions().dtype(torch::kF32).device(device);

  const int batch_count = std::thread::hardware_concurrency();
  const int data_size = flow_list.size();
  const int batch_size = (flow_list.size() + batch_count - 1) / batch_count;

  using future_type = std::pair<std::vector<torch::Tensor>, std::vector<torch::Tensor>>;
  std::vector<std::future<future_type>> futures;
  futures.reserve(batch_count);
  for (int i = 0; i < data_size; i += batch_size) {
    futures.emplace_back(std::async(std::launch::async, [&, start=i, end=std::min(i + batch_size, data_size)] {
      std::vector<torch::Tensor> _window_list;
      std::vector<torch::Tensor> _index_list;
      _window_list.reserve(batch_size);
      _index_list.reserve(batch_size);

      long offset = 0;
      for (int j = start; j < end; ++j) {
        hd_flow& flow = flow_list[j];
        torch::Tensor flow_tensor = FlowToTensor(flow, window_option, device, width);
        auto _windows = flow_tensor.reshape({-1, 5, 136}).slice(1, 0, 4).reshape({-1, 544});
        _window_list.emplace_back(_windows);
        _index_list.emplace_back(torch::tensor({offset, offset + _windows.size(0)}, index_option));
        offset += _windows.size(0);
      }
      return std::make_pair(_window_list, _index_list);
    }));
  }
  std::vector<torch::Tensor> combined_window_list;
  std::vector<torch::Tensor> combined_index_list;
  for (auto& future : futures) {
    auto [window_list, index_list] = future.get();
    combined_window_list.insert(combined_window_list.end(), window_list.begin(), window_list.end());
    combined_index_list.insert(combined_index_list.end(), index_list.begin(), index_list.end());
  }
  const auto input_window = torch::concat(combined_window_list, 0).to(device);
  const auto index_tensor = torch::stack(combined_index_list, 0).to(device);
  return std::make_pair(z_score_norm(input_window, device), index_tensor);
}

std::tuple<torch::Tensor, torch::Tensor>
transform::BuildSlideWindow(flow_vec_ref const& flow_list, int width, torch::Device& device) {
  const auto index_option = torch::TensorOptions().dtype(torch::kI32).device(device);
  const auto window_option = torch::TensorOptions().dtype(torch::kF32).device(device);

  long num_windows = 0;
  std::ranges::for_each(flow_list, [&num_windows, width](hd_flow const& flow) {
    num_windows += flow.count / width;
  });
  const long flow_list_len = flow_list.size();

  std::vector<torch::Tensor> index_list;
  index_list.reserve(flow_list_len);

  std::vector<torch::Tensor> window_list;
  window_list.reserve(num_windows);

  long offset = 0;
  std::ranges::for_each(flow_list, [&](hd_flow& flow) {
    const torch::Tensor flow_tensor = FlowToTensor(flow, window_option, device, width);
    auto viewd = flow_tensor.reshape({-1, 5, 136}).slice(1, 0, 4).reshape({-1, 544});
    window_list.emplace_back(viewd);
    index_list.emplace_back(torch::tensor({offset, offset + viewd.size(0)}, index_option));
    offset += viewd.size(0);
  });

  const auto input_window = torch::concat(window_list, 0).to(device);
  const auto index_tensor = torch::stack(index_list, 0).to(device);
  return std::make_tuple(z_score_norm(input_window, device), index_tensor);
}

torch::Tensor
transform::MergeFlow(torch::Tensor const& predict_flows, torch::Tensor const& index_arr, torch::Device& device) {
  const auto options = torch::TensorOptions().dtype(torch::kFloat32).device(device);
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
  const auto temp = tensor.cpu();
  auto tensor_accessor = temp.accessor<int32_t, 2>();  // 使用 float 类型和 2D 访问器
  for (int i = 0; i < std::min(10, (int)tensor_accessor.size(0)); ++i) {
    std::cout << "(" << tensor_accessor[i][0] << ", " << tensor_accessor[i][1] << ")\n";
  }
}

torch::Tensor
BatchEncode(torch::jit::Module* model, const torch::Tensor& data,
            int64_t batch_size, int64_t max_batch, bool stay_on_gpu) {
  const int64_t data_size = data.size(0);
  std::vector<torch::Tensor> CPU_results, GPU_results;
  GPU_results.reserve(data_size / batch_size + 1);
  CPU_results.reserve(data_size / batch_size + 1);
  for (int64_t start = 0; start < data_size; start += batch_size) {
    auto end = std::min(start + batch_size, data_size);
    auto batch_data = data.slice(0, start, end);
    auto output = model->forward({data}).toTensor();
    GPU_results.emplace_back(output);
    if (stay_on_gpu) [[unlikely]] continue;
    // ↓↓↓ 把数据转移至CPU防止GPU-OOM
    if (GPU_results.size() < max_batch) continue;
    CPU_results.emplace_back(concat(GPU_results, 0).cpu());
    GPU_results.clear();
  }
  if (stay_on_gpu) [[unlikely]] return concat(GPU_results, 0);//.to(hd::global::calc_device);
  if (not GPU_results.empty()) {
    CPU_results.emplace_back(concat(GPU_results, 0).cpu());
    GPU_results.clear();
  }
  return concat(CPU_results, 0);
}

#pragma endregion Exported API
