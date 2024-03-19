//
// Created by brian on 3/11/24.
//

#ifndef ENCODER_ENCODER_HPP
#define ENCODER_ENCODER_HPP

#include <torch/torch.h>

using namespace torch;

class EmbeddingStock : public torch::nn::Embedding {
public:
  EmbeddingStock(int64_t input_size, int64_t max_length);

  auto forward(const torch::Tensor& input);

private:
  int64_t input_size;
  int64_t embedding_dim;
  int64_t max_length;
};

class Encoder : public torch::nn::Module {
private:
  int64_t hidden_size;
  int64_t num_layers;
  bool bidirectional;
  torch::nn::Embedding embedding;
  torch::nn::GRU recurrent;
public:
  Encoder(torch::nn::Embedding& embedding, int64_t hidden_size,
          int64_t num_layers = 1, bool bidirectional = false);

  std::pair<torch::Tensor, torch::Tensor> forward(const torch::Tensor& input);
};

class DecoderEvent : public torch::nn::Module {
private:
  torch::nn::Linear hidden;
  torch::nn::Linear output;
  torch::nn::Dropout dropout;

public:
  DecoderEvent(int64_t input_size, int64_t output_size, double _dropout = 0.1, std::string name = "DecoderEvent");

  auto forward(const torch::Tensor& x, const torch::Tensor& attention);
};

class DecoderAttention : public torch::nn::Module {
private:
  torch::nn::Embedding embedding;
  torch::nn::GRU recurrent;
  torch::nn::LSTM lstm;
  torch::nn::Linear attn;
  torch::nn::Dropout dropout;
public:
  DecoderAttention(torch::nn::Embedding& embedding, int64_t context_size,
                   int64_t attention_size, int64_t num_layers, double dropout_value,
                   bool bidirectional);

  auto forward(const Tensor& input, Tensor const& hidden_state);

};

class ContextBuilder : torch::nn::Module {
private:
  torch::nn::Embedding embedding{nullptr};
  torch::nn::Embedding embeddingStock{nullptr};
  std::shared_ptr<Encoder> encoder{nullptr};
  std::shared_ptr<DecoderAttention> decoder_attention{nullptr};
  std::shared_ptr<DecoderEvent> decoder_event{nullptr};
public:
  ContextBuilder(int64_t input_size, int64_t output_size, int64_t _layers = 1,
                 int64_t max_length = 10, bool bi = false, double dropout = 0.1, int64_t hidden_size = 128);

  auto forward(const torch::Tensor& X);
};


#endif //ENCODER_ENCODER_HPP
