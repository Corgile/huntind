#include <iostream>

#include "hound/encoding/flow-encode.hpp"
#include "hound/encoding/detail/encoder.hpp"
#include "hound/encoding/detail/transform.hpp"

#pragma region EmbeddingStock, checked

EmbeddingStock::EmbeddingStock() : torch::nn::Embedding(nullptr) {}

EmbeddingStock::EmbeddingStock(int64_t input_size, int64_t max_length)
  : torch::nn::Embedding(torch::nn::EmbeddingOptions(input_size, input_size).padding_idx(0)),
    input_size(input_size),
    embedding_dim(input_size),
    max_length(max_length) {
}
auto EmbeddingStock::forward(Tensor const& batch_edge) {
  std::vector<torch::Tensor> concat_embed;
  concat_embed.reserve(max_length);
  for (int64_t i = 0; i < max_length; ++i) {
    auto packet = batch_edge.slice(1, i * embedding_dim, (i + 1) * embedding_dim);
    concat_embed.emplace_back(packet);
  }
  return torch::cat(concat_embed, 1)
    .view({batch_edge.size(0), max_length, embedding_dim})
    .to(torch::kFloat);
}

#pragma endregion EmbeddingStock

#pragma region Encoder, checked

Encoder::Encoder(torch::nn::Embedding& embedding, int64_t hidden_size, int64_t num_layers, bool bidirectional) {
  hidden_size = hidden_size;
  num_layers = num_layers;
  bidirectional = bidirectional;
  embedding = embedding;
  auto opt = torch::nn::GRUOptions(embedding->options.embedding_dim(), hidden_size)
    .num_layers(num_layers)
    .batch_first(true)
    .bidirectional(bidirectional);
  register_module("recurrent", torch::nn::GRU(std::move(opt)));
}

std::pair<torch::Tensor, torch::Tensor> Encoder::forward(Tensor const& input) {
  auto embedded = embedding(input);
  auto opt = torch::TensorOptions().device(input.device());
  auto hx = torch::zeros({num_layers * (1 + bidirectional), input.size(0), hidden_size}, std::move(opt));
  auto [_, hidden_state] = recurrent(embedded, std::move(hx));
  if (bidirectional) {
    embedded = torch::cat({embedded, embedded}, /*dim=*/2);
  }
  return {embedded, hidden_state};
}

Encoder::Encoder() {}

#pragma endregion Encoder

#pragma region DecoderEvent, checked

DecoderEvent::DecoderEvent(int64_t input_size, int64_t output_size, double _dropout, std::string name)
  : Module(std::move(name)) {
  hidden = torch::nn::Linear(input_size, input_size);
  output = torch::nn::Linear(input_size, output_size);
  dropout = torch::nn::Dropout(_dropout);
  register_module("hidden", hidden);
  register_module("output", output);
  register_module("dropout", dropout);
}

auto DecoderEvent::forward(Tensor const& x, Tensor const& attention) {
  auto attn_applied = torch::bmm(attention.unsqueeze(1), x).squeeze(1);
  auto out = this->hidden(attn_applied).relu();
  return this->output(out);
}

DecoderEvent::DecoderEvent() {}

#pragma endregion DecoderEvent

#pragma region DecoderAttention, checked

DecoderAttention::DecoderAttention(torch::nn::Embedding& embedding, int64_t context_size,
                                   int64_t attention_size, int64_t num_layers, double dropout_value,
                                   bool bidirectional) {
  register_module("embedding", embedding);
  register_module("attention", torch::nn::Linear(context_size * num_layers * (1 + bidirectional), attention_size));
  register_module("dropout", torch::nn::Dropout(dropout_value));

  torch::nn::GRUOptions rnn_options(embedding->options.embedding_dim(), context_size);
  rnn_options.num_layers(num_layers).batch_first(true).bidirectional(bidirectional);
  register_module("recurrent", torch::nn::GRU(rnn_options));
}

auto DecoderAttention::forward(Tensor const& input, Tensor const& hidden_state) {
  Tensor embedded = dropout(embedding(input).view({-1, 1, embedding->options.embedding_dim()}));
  auto [attention, context_vector] = recurrent->forward(embedded, hidden_state);
  auto flow_hidden = attention.squeeze(1);
  attention = this->attn(attention.squeeze(1));
  attention = torch::softmax(attention, 1);
  return std::make_tuple(attention, context_vector, flow_hidden);
}

#pragma endregion DecoderAttention

#pragma region ContextBuilder, checked

ContextBuilder::ContextBuilder(int64_t input_size, int64_t output_size, int64_t _layers,
                               int64_t max_length, bool bi, double dropout, int64_t hidden_size) {
  register_module("embedding", torch::nn::Embedding(input_size, hidden_size));
  register_module("embeddingStock", EmbeddingStock(input_size, max_length));

  encoder = std::make_shared<Encoder>(embeddingStock, hidden_size, _layers, bi);
  decoder_attention = std::make_shared<DecoderAttention>(embedding, hidden_size, max_length, _layers, dropout, bi);
  decoder_event = std::make_shared<DecoderEvent>(input_size, output_size, dropout);

  register_module("encoding", encoder);
  register_module("decoder_attention", decoder_attention);
  register_module("decoder_event", decoder_event);
}

auto ContextBuilder::forward(Tensor const& X) {
  auto decoder_input = torch::zeros({X.size(0), 1}, torch::dtype(torch::kLong).device(X.device()));
  auto [X_encoded, context_vector] = encoder->forward(X);
  auto [attention_, new_context_vector, flow_hidden] =
    decoder_attention->forward(context_vector, decoder_input);
  auto confidence_ = decoder_event->forward(X_encoded, attention_);
  return std::make_tuple(confidence_, flow_hidden);
}

ContextBuilder::ContextBuilder() {}

#pragma endregion ContextBuilder

#pragma region transform

torch::Tensor transform::z_score_norm(at::Tensor& data) {
  if (data.dim() == 1) data = data.unsqueeze(1);
  Tensor mean = torch::nanmean(data, /*dim=*/1, /*keepdim=*/true);
  /// standard deviation; should be torch::nanstd
  Tensor std = torch::nansum(data, /*dim=*/1, /*keepdim=*/true);
  std = torch::where(std == 0, torch::tensor(1e-8, data.options()), std);
  Tensor normalized_data = torch::nan_to_num((data - mean) / std);
  if (data.dim() == 2 and data.size(1) == 1) {
    normalized_data = normalized_data.squeeze(1);
  }
  return normalized_data;
}

Tensor transform::convert_to_npy(hd_flow const& flow) {
  std::vector<Tensor> flow_data_list;
  flow_data_list.reserve(flow.count);
  int tensor_shape = flow._packet_list[0].raw.length();
  for (auto& packet : flow._packet_list) {
    void* array = (void*) packet.raw.data();
    Tensor tensor_data = torch::from_blob(array, {tensor_shape}, torch::kU8);
    Tensor processed_data;
    if (flow.protocol == IPPROTO_TCP) {
      processed_data = torch::cat(
        {
          tensor_data.slice(0, 0, 12),
          tensor_data.slice(0, 20, 60),
          tensor_data.slice(0, 64)
        });
    } else {
      processed_data = torch::cat(
        {
          tensor_data.slice(0, 0, 12),
          tensor_data.slice(0, 20, 120),
          tensor_data.slice(0, 124)
        });
    }
    flow_data_list.emplace_back(processed_data);
  }
  return torch::cat(flow_data_list, 0).to(torch::kU8);
}

/// @brief 构建滑动窗口
/// @param flow_list Tensor in shape of (flow_count, (flow_shape))
/// @param win_stride slide window stride
/// @param actual_pkt_len  I don't know neither
/// @return tuple of 2 tensors representing <code>window_arr</code> and <code>flow_index_arr</code>
std::tuple<torch::Tensor, torch::Tensor>
transform::build_slide_window(Tensor const& flow_list, int win_stride, int actual_pkt_len) {
  std::vector<Tensor> window_list;
  std::vector<Tensor> flow_indices;
  int window_index = 0;

  for (const Tensor& flow : flow_list.chunk(flow_list.size(0), 0)) {
    if (flow.size(0) < win_stride) continue;
    int window_start_index = window_index;
    for (int start = 0; start <= flow.size(0) - win_stride; ++start) {
      window_list.push_back(flow.slice(0, start, start + win_stride).flatten());
      window_index++;
    }
    if (window_index != window_start_index) {
      flow_indices.push_back(torch::tensor({window_start_index, window_index}, torch::kInt32));
    }
  }
  auto window_arr = torch::stack(window_list).toType(torch::kFloat32);
  auto flow_index_arr = torch::stack(flow_indices).toType(torch::kInt32);
  window_arr = transform::z_score_norm(window_arr);
  window_arr = window_arr.slice(1, 0, window_arr.size(1) - actual_pkt_len);
  return std::make_pair(window_arr, flow_index_arr);
}

torch::Tensor
transform::merge_flow(Tensor const& predict_flows, torch::Tensor const& flow_indices) {
  auto num_flows = flow_indices.size(0);
  auto merged_flows = torch::zeros({num_flows, predict_flows.size(1)}, predict_flows.options());
  torch::nn::AdaptiveMaxPool1d adaptive_max_pool1d(torch::nn::AdaptiveMaxPool1dOptions(1));
  for (int64_t i = 0; i < num_flows; ++i) {
    auto start = flow_indices[i][0].item<int64_t>();
    auto end = flow_indices[i][1].item<int64_t>();
    auto merged_flow = predict_flows.slice(0, start, end);
    merged_flow = merged_flow.transpose(0, 1).unsqueeze(0);
    auto pooled = adaptive_max_pool1d(merged_flow);
    // 移除不必要的维度并分配到结果张量中
    merged_flows[i] = pooled.squeeze();
  }
  return merged_flows;
}

#pragma endregion transform

#pragma region Exported API

[[maybe_unused]]
torch::Tensor encode(const hd::type::hd_flow& msg) {
  auto [
    flow_encode_model,
    origin_packet_length,
    num_window_packets,
    batch_size,
    calc_device
  ] = load_model_config("/path");
  torch::Tensor flow_data = transform::convert_to_npy(msg);
  auto [window_arr, flow_index_arr] = transform::build_slide_window(flow_data, num_window_packets, msg.count);
  auto encoded_flows = batch_model_encode(flow_encode_model, flow_data, batch_size);
  return transform::merge_flow(encoded_flows, flow_index_arr).detach().cpu();
}

[[maybe_unused]] std::tuple<torch::jit::script::Module, int, int, int, torch::Device>
load_model_config(std::string& encodeModelPath) {
  auto deviceType = torch::kCUDA;
  int deviceId = 0;
  int batchSize = 8192;
  int attentionSize = 128;
  int numWindowPackets = 5;
  int payloadSize = 20;

  torch::Device device(deviceType, deviceId);
  if (encodeModelPath.empty()) {
    encodeModelPath.assign("./models/flow_encoder/encoder_")
      .append(std::to_string(attentionSize)).append("_")
      .append(std::to_string(payloadSize)).append("_")
      .append(std::to_string(numWindowPackets))
      .append("_notime_noip_noport.pt");
  }
  int originPacketLength = 128 + payloadSize; // 需要设定包头长度
  torch::jit::script::Module flowEncodeModel;
  try {
    flowEncodeModel = torch::jit::load(encodeModelPath);
    flowEncodeModel.to(device);
    flowEncodeModel.eval();
  } catch (const c10::Error& e) {
    std::cerr << "模型加载错误: " << e.msg() << std::endl;
  }

  return std::make_tuple(std::move(flowEncodeModel), originPacketLength, numWindowPackets, batchSize, device);
}

[[maybe_unused]] torch::Tensor
batch_model_encode(jit::Module& model,
                   torch::Tensor data,
                   int64_t batch_size,
                   int64_t max_num_batches,
                   bool retain) {
  model.eval();
  auto data_length = data.size(0);
  std::vector<torch::Tensor> results, results_cpu;
  int64_t batch_count = 0;

  if (data_length > batch_size) {
    for (int64_t start = 0; start < data_length; start += batch_size) {
      auto end = std::min(start + batch_size, data_length);
      auto batch_data = data.slice(0, start, end);
      auto output = model.forward({batch_data}).toTensor().detach();
      results.push_back(output);

      batch_count++;
      if (!retain && batch_count > max_num_batches) {
        batch_count = 0;
        results_cpu.push_back(torch::cat(results, 0).cpu());
        results.clear();
      }
    }
    if (retain) {
      return torch::cat(results, 0);
    }
    if (batch_count > 0) {
      results_cpu.push_back(torch::cat(results, 0).cpu());
    }
    if (results_cpu.size() == 1) {
      return results_cpu[0];
    } else {
      return torch::cat(results_cpu, 0);
    }
  } else {
    auto output = model.forward({data}).toTensor().detach();
    if (!retain) {
      output = output.cpu();
    }
    return output;
  }
}

#pragma endregion Exported API