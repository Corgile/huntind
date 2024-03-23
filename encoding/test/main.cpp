//
// hound-torch / main.cpp.
// Created by brian on 2024-03-20.
//

#include <random>
#include <my-timer.hpp>
#include "flow-encode.hpp"

hd::type::parsed_packet genrate() {
  std::random_device rd; // Obtain a random number from hardware
  std::mt19937 gen(rd()); // Seed the generator
  std::uniform_int_distribution distr(0, 255); // Define the range;
  std::string blob;
  for (int i = 0; i < 148; ++i) {
    blob.push_back(distr(gen));
  }
  hd::type::parsed_packet _packet;
  _packet.HasContent = true;
  _packet.protocol = 6;
  _packet.mTsSec = 1723235588;
  _packet.mTSuSec = 23641;
  _packet.mKey = "127.0.0.1_192.168.8.124_22_4522_TCP";
  _packet.mBlobData = blob;
  std::uniform_int_distribution dist(60, 1255);
  _packet.mCapLen = dist(gen);
  return _packet;
}

int main(int argc, char* argv[]) {
  hd::type::hd_flow _flow;
  _flow.count = 10;
  _flow.protocol = IPPROTO_TCP;
  _flow.flowId = "127.0.0.1_192.168.8.124_22_4522_TCP";
  _flow._packet_list.reserve(10);
  for (int i = 0; i < 10; ++i) {
    _flow._packet_list.emplace_back(genrate());
  }
  std::vector<hd::type::hd_flow> flow_list;
  for (int i = 0; i < 20; ++i) {
    flow_list.emplace_back(_flow);
  }
  xhl::Timer t("流编码测试");
  const auto encoded = encode(flow_list);
  torch::print(encoded);
  print_shape(encoded);
  return 0;
}
