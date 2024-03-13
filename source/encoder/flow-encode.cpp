#include <iostream>

#include <hound/encoding/flow-encode.hpp>
#include <hound/encoding/detail/encoder.hpp>
#include <hound/encoding/detail/transform.hpp>

#pragma region EmbeddingStock

EmbeddingStock::EmbeddingStock(int64_t input_size, int64_t max_length) {
  input_size = input_size;
  embedding_dim = input_size;
  max_length = max_length;
}

auto EmbeddingStock::forward(Tensor const& batch_edge) {
  std::vector<torch::Tensor> concat_embed;
  for (int64_t i = 0; i < max_length; ++i) {
    auto packet = batch_edge.slice(1, i * embedding_dim, (i + 1) * embedding_dim);
    concat_embed.push_back(packet);
  }
  return torch::cat(concat_embed, 1)
    .view({batch_edge.size(0), max_length, embedding_dim})
    .to(torch::kFloat);
}

#pragma endregion EmbeddingStock

#pragma region Encoder

Encoder::Encoder(nn::Embedding& embedding, int64_t hidden_size, int64_t num_layers, bool bidirectional, bool LSTM) {
  hidden_size = hidden_size;
  num_layers = num_layers;
  bidirectional = bidirectional;
  embedding = embedding;
  // 根据是否使用LSTM来初始化循环层
  if (LSTM) {
    auto opt = torch::nn::LSTMOptions(embedding->options.embedding_dim(), hidden_size);
    opt.num_layers(num_layers).batch_first(true).bidirectional(bidirectional);
    lstm = register_module("recurrent", torch::nn::LSTM(opt));
  } else {
    gru = register_module("recurrent", torch::nn::GRU(
      torch::nn::GRUOptions(embedding->options.embedding_dim(), hidden_size)
        .num_layers(num_layers).batch_first(true).bidirectional(bidirectional)));
  }
}

std::pair<torch::Tensor, torch::Tensor> Encoder::forward(Tensor const& input) {
  auto embedded = embedding(input);
  // 初始化隐藏状态为0
  auto hx = torch::zeros({num_layers * (1 + bidirectional), input.size(0), hidden_size},
                         torch::TensorOptions().device(input.device()));
  auto output = gru(embedded, hx);

  torch::Tensor hidden_state = std::get<1>(output);

  if (bidirectional) {
    // 如果是双向的，这里可能需要根据你的具体需求来调整输出的处理方式
    embedded = torch::cat({embedded, embedded}, /*dim=*/2);
  }

  return {embedded, hidden_state};
}

#pragma endregion Encoder

#pragma region DecoderEvent

DecoderEvent::DecoderEvent(int64_t input_size, int64_t output_size, double d, std::string name)
  : Module(std::move(name)) {
  hidden = torch::nn::Linear(input_size, input_size);
  output = torch::nn::Linear(input_size, output_size);
  dropout = torch::nn::Dropout(d);
  register_module("hidden", hidden);
  register_module("output", output);
  register_module("dropout", dropout);
}

auto DecoderEvent::forward(Tensor const& x, Tensor const& attention) {
  auto attn_applied = torch::bmm(attention.unsqueeze(1), x).squeeze(1);
  auto out = this->hidden(attn_applied).relu();
  return this->output(out);
}

#pragma endregion DecoderEvent

#pragma region DecoderAttention

DecoderAttention::DecoderAttention(int64_t vocab_size, int64_t embedding_dim, int64_t context_size,
                                   int64_t attention_size, int64_t num_layers, double dropout_value, bool bidirectional,
                                   bool LSTM) {
  embedding = register_module("embedding", torch::nn::Embedding(vocab_size, embedding_dim));
  attn = register_module("attn", torch::nn::Linear(context_size * num_layers * (1 + bidirectional), attention_size));
  dropout = register_module("dropout", torch::nn::Dropout(dropout_value));
  use_lstm = LSTM;
  // 根据LSTM标志选择初始化GRU或LSTM
  if (use_lstm) {
    torch::nn::LSTMOptions rnn_options(embedding_dim, context_size);
    rnn_options.num_layers(num_layers).batch_first(true).bidirectional(bidirectional);
    lstm = register_module("recurrent", torch::nn::LSTM(rnn_options));
  } else {
    torch::nn::GRUOptions rnn_options(embedding_dim, context_size);
    rnn_options.num_layers(num_layers).batch_first(true).bidirectional(bidirectional);
    gru = register_module("recurrent", torch::nn::GRU(rnn_options));
  }
}

auto DecoderAttention::forward(Tensor const& input, std::tuple<Tensor, Tensor> hidden_state) {
  Tensor embedded = dropout(embedding(input).view({-1, 1, embedding->options.embedding_dim()}));

  Tensor output;
  std::tuple<Tensor, Tensor> new_hidden_state;

  if (use_lstm) {
    // 对于LSTM，如果hidden_state是空的，我们不需要手动初始化它，LSTM内部会处理
    auto lstm_output = lstm->forward(embedded, hidden_state);
    output = std::get<0>(lstm_output); // LSTM输出
    new_hidden_state = std::get<1>(lstm_output); // 新的(hidden state, cell state)
  } else {
    // 对于GRU，如果hidden_state不是空的，我们只需要传递hidden state部分
    auto gru_output = gru->forward(embedded, std::get<0>(hidden_state));
    output = std::get<0>(gru_output); // GRU输出
    new_hidden_state = std::make_tuple(std::get<1>(gru_output), Tensor()); // 新的hidden state, 第二个元素为空Tensor
  }

  // 对输出应用attention层
  auto attn_output = attn(output.squeeze(1));
  auto attention = torch::softmax(attn_output, 1);

  // 返回计算得到的attention、更新后的隐藏状态和原始的RNN输出
  return std::make_tuple(attention, new_hidden_state, output);
}

#pragma endregion DecoderAttention

#pragma region ContextBuilder

ContextBuilder::ContextBuilder(int64_t input_size, int64_t output_size, int64_t hidden_size, int64_t _layers,
                               int64_t max_length, bool bi, bool LSTM, double dropout) {
  embedding = register_module("embedding", torch::nn::Embedding(input_size, hidden_size));
  // embeddingStock = register_module("embeddingStock", EmbeddingStock(input_size, max_length));
  encoder = std::make_shared<Encoder>(embedding, hidden_size, _layers, bi, LSTM);
  decoder_attention = std::make_shared<DecoderAttention>(hidden_size, max_length, _layers, dropout, bi, LSTM);
  decoder_event = std::make_shared<DecoderEvent>(input_size, output_size, dropout);
  // TODO: 这玩意到底咋传参
  // register_module("encoding", encoder);
  register_module("decoder_attention", decoder_attention);
  register_module("decoder_event", decoder_event);
}

auto ContextBuilder::forward(Tensor const& X) {
  auto decoder_input = torch::zeros({X.size(0), 1}, torch::dtype(torch::kLong).device(X.device()));
  auto [X_encoded, context_vector] = encoder->forward(X);
  auto [attention_, new_context_vector, flow_hidden] =
    decoder_attention->forward(context_vector, {decoder_input, decoder_input});
  auto confidence_ = decoder_event->forward(X_encoded, attention_);
  return std::make_tuple(confidence_, flow_hidden);
}

#pragma endregion ContextBuilder

[[maybe_unused]] Tensor encode() {
  // 假设loadModelConfig是之前定义的加载模型和配置的函数
  auto [flowEncodeModel, originPacketLength, numWindowPackets, batchSize, device] = loadModelConfig();
  // 模拟获取消息
  hd::type::hd_flow dataProducer; // 需要填充数据
  auto flowDataList = transform::convert_to_npy(dataProducer, originPacketLength);
  auto [windowArr, flowIndexArr] = transform::build_slide_window(flowDataList, numWindowPackets, originPacketLength);
  // 批量流编码
  std::vector<Tensor> encodedFlows;
  // for (auto& window : windowArr) {
  //   auto encodedFlow = flowEncodeModel.forward({window.to(device)}).toTensor();
  //   encodedFlows.push_back(encodedFlow);
  // }
  // 合并流编码结果
  return transform::merge_flow(encodedFlows, flowIndexArr);
}

[[maybe_unused]] std::tuple<torch::jit::script::Module, int, int, int, torch::Device>
loadModelConfig(string const& encodeModelPath) {
  // 模型配置
  auto deviceType = torch::kCUDA; // 使用CUDA
  int deviceId = 0; // CUDA设备ID
  int batchSize = 8192;
  int attentionSize = 128;
  int numWindowPackets = 5;
  int payloadSize = 20;

  torch::Device device(deviceType, deviceId);

  // 根据实际情况设置模型路径
  std::string encoderModelPath;
  if (!encodeModelPath.empty()) {
    encoderModelPath = encodeModelPath;
  } else {
    encoderModelPath = "./models/flow_encoder/encoder_" + std::to_string(attentionSize) + "_" +
                       std::to_string(payloadSize) + "_" + std::to_string(numWindowPackets) + "_notime_noip_noport.pt";
  }

  int originPacketLength = /*encoderConfig['packet_header_length']*/ +payloadSize; // 需要设定包头长度
  int realPacketLength = originPacketLength - 12; // 去掉IP和端口

  // 加载模型
  torch::jit::script::Module flowEncodeModel;
  try {
    // 加载预训练的模型
    flowEncodeModel = torch::jit::load(encoderModelPath);
    flowEncodeModel.to(device);
    flowEncodeModel.eval();
  } catch (const c10::Error& e) {
    std::cerr << "模型加载错误: " << e.msg() << std::endl;
  }

  return std::make_tuple(std::move(flowEncodeModel), originPacketLength, numWindowPackets, batchSize, device);
}

torch::Tensor batch_model_encode(jit::Module& model, torch::Tensor data, int64_t batch_size, torch::Device device,
                                 int64_t max_num_batches, bool result_stay_on_device) {
  model.eval();
  auto data_length = data.size(0);
  std::vector<torch::Tensor> results, results_cpu;
  int64_t batch_count = 0;

  if (data_length > batch_size) {
    for (int64_t start = 0; start < data_length; start += batch_size) {
      auto end = std::min(start + batch_size, data_length);
      auto batch_data = data.slice(0, start, end).to(device);
      auto output = model.forward({batch_data}).toTensor().detach();
      results.push_back(output);

      batch_count++;
      if (!result_stay_on_device && batch_count > max_num_batches) {
        batch_count = 0;
        results_cpu.push_back(torch::cat(results, 0).cpu());
        results.clear();
      }
    }

    if (result_stay_on_device) {
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
    auto output = model.forward({data.to(device)}).toTensor().detach();
    if (!result_stay_on_device) {
      output = output.cpu();
    }
    return output;
  }
}

#pragma region transform

at::Tensor transform::z_score_norm(at::Tensor& data) {
  // 确保数据为二维
  if (data.dim() == 1) data = data.unsqueeze(1);
  Tensor mean = torch::nanmean(data, /*dim=*/1, /*keepdim=*/true);
  /// standard deviation; should be torch::nanstd
  Tensor std = torch::nansum(data, /*dim=*/1, /*keepdim=*/true);
  // 将标准差为0的值替换为一个很小的值
  std = torch::where(std == 0, torch::tensor(1e-8, data.options()), std);
  Tensor normalized_data = torch::nan_to_num((data - mean) / std);
  if (data.dim() == 2 and data.size(1) == 1) {
    normalized_data = normalized_data.squeeze(1);
  }
  return normalized_data;
}

std::vector<torch::Tensor> transform::convert_to_npy(hd::type::hd_flow const& msg, int packet_length) {
  std::vector<Tensor> flow_data_list;
  std::string protocol = msg.flowId.substr(msg.flowId.rfind('_'));
  for (auto& packet_data : msg.data) {
    // 这里假设bitvec是以逗号分隔的数字字符串
    std::vector<uint8_t> raw_data;
    std::stringstream ss(packet_data.bitvec);
    std::string item;
    while (std::getline(ss, item, ',')) {
      raw_data.push_back(static_cast<uint8_t>(std::stoi(item)));
    }
    Tensor tensor_data = torch::from_blob(raw_data.data(), {packet_length}, torch::kUInt8);
    Tensor processed_data;
    if (protocol == "TCP") {
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
    flow_data_list.push_back(processed_data);
  }
  Tensor flow_info; // 根据需要定义
  Tensor flow_data_arr = torch::stack(flow_data_list);
  return {flow_info, flow_data_arr};
}

std::tuple<torch::Tensor, torch::Tensor>
transform::build_slide_window(std::vector<Tensor> const& flow_list, int win_size, int actual_pkt_len) {
  std::vector<Tensor> window_list;
  std::vector<Tensor> flow_index_list;
  int window_index = 0;
  for (const Tensor& flow : flow_list) {
    if (flow.size(0) < win_size) continue; // 丢弃包数量比滑动窗口短的流
    int window_start_index = window_index;
    for (int start = 0; start <= flow.size(0) - win_size; ++start) {
      // 使用flatten()来展平张量，相当于numpy的ravel()
      window_list.push_back(flow.slice(0, start, start + win_size).flatten());
      window_index++;
    }
    if (window_index != window_start_index) {
      // 使用torch::tensor创建索引张量
      flow_index_list.push_back(torch::tensor({window_start_index, window_index}, torch::dtype(torch::kInt32)));
    }
  }
  // 使用torch::stack将vector<Tensor>转换为一个2D张量
  auto window_arr = torch::stack(window_list);
  auto flow_index_arr = torch::stack(flow_index_list);
  // 转换数据类型
  window_arr = window_arr.toType(torch::kFloat32);
  flow_index_arr = flow_index_arr.toType(torch::kInt32);
  // Z-score标准化
  window_arr = transform::z_score_norm(window_arr); // 假设已经实现
  // 去掉滑动窗口中最后一个包
  window_arr = window_arr.slice(1, 0, window_arr.size(1) - actual_pkt_len);
  return std::make_pair(window_arr, flow_index_arr);
}

at::Tensor transform::merge_flow(std::vector<Tensor> const& predict_flows, at::Tensor const& flow_indices) {
  // 检查输入参数
  using namespace torch;
  TORCH_CHECK(predict_flows[0].dim() == 2, "predict_flows must be a 2D tensor")
  TORCH_CHECK(flow_indices.dim() == 2, "flow_indices must be a 2D tensor")
  TORCH_CHECK(flow_indices.size(1) == 2, "flow_indices must have 2 columns")
  Tensor merged_flows = torch::zeros(
    {flow_indices.size(0), predict_flows[0].size(1)},
    predict_flows[0].options()
  );
  for (int64_t i = 0; i < flow_indices.size(0); ++i) {
    long start = flow_indices[i][0].item<int64_t>();
    long end = flow_indices[i][1].item<int64_t>();
    Tensor _flow = predict_flows[0].slice(0, start, end).transpose(0, 1);
    merged_flows[i] = nn::functional::adaptive_max_pool1d(_flow.unsqueeze(0),
                                                          nn::AdaptiveMaxPool1dOptions(1)).squeeze();
  }
  return merged_flows;
}

#pragma endregion transform