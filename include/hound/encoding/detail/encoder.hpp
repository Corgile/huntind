//
// Created by brian on 3/11/24.
//

#ifndef ENCODER_ENCODER_HPP
#define ENCODER_ENCODER_HPP
#include <torch/torch.h>
using namespace torch;

class EmbeddingStock : public torch::nn::Embedding {
private:
  int64_t input_size;
  int64_t embedding_dim;
  int64_t max_length;
public:
  // 构造函数
  EmbeddingStock(int64_t input_size, int64_t max_length);

  // 前向传播函数
  auto forward(const torch::Tensor& batch_edge);
};

class Encoder : torch::nn::Module {
private:
  int64_t hidden_size;
  int64_t num_layers;
  bool bidirectional;
  torch::nn::Embedding embedding;
  torch::nn::GRU gru;
  torch::nn::LSTM lstm;
public:
  Encoder(torch::nn::Embedding& embedding, int64_t hidden_size,
          int64_t num_layers = 1, bool bidirectional = false,
          bool LSTM = false);

  std::pair<torch::Tensor, torch::Tensor> forward(const torch::Tensor& input);
};

class DecoderEvent : public torch::nn::Module {
private:
  torch::nn::Linear hidden;
  torch::nn::Linear output;
  torch::nn::Dropout dropout;

public:
  DecoderEvent(int64_t input_size, int64_t output_size, double d = 0.1, std::string name = "DecoderEvent");

  auto forward(const torch::Tensor& x, const torch::Tensor& attention);
};

class DecoderAttention : public torch::nn::Module {
private:
  torch::nn::Embedding embedding;
  torch::nn::GRU gru;
  torch::nn::LSTM lstm;
  torch::nn::Linear attn;
  torch::nn::Dropout dropout;
  bool use_lstm;
public:
  DecoderAttention(int64_t vocab_size, int64_t embedding_dim, int64_t context_size, int64_t attention_size,
                   int64_t num_layers = 1, double dropout_value = 0.1, bool bidirectional = false, bool LSTM = false);

  auto forward(const Tensor& input, std::tuple<Tensor, Tensor> hidden_state = {});

};

class ContextBuilder : torch::nn::Module {
private:
  torch::nn::Embedding embedding{nullptr};
  std::shared_ptr<Encoder> encoder{nullptr};
  std::shared_ptr<DecoderAttention> decoder_attention{nullptr};
  std::shared_ptr<DecoderEvent> decoder_event{nullptr};
public:
  ContextBuilder(int64_t input_size, int64_t output_size, int64_t hidden_size = 128, int64_t _layers = 1,
                 int64_t max_length = 10, bool bi = false, bool LSTM = false, double dropout = 0.1);

  auto forward(const torch::Tensor& X);
};



// TORCH_MODULE(Encoder); // 为EncoderImpl提供一个方便的接口


#endif //ENCODER_ENCODER_HPP
