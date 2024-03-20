#include <random>

#include "flow-encode.hpp"
//
// hound-torch / main.cpp. 
// Created by brian on 2024-03-20.
//
hd::type::parsed_packet genrate() {
  std::random_device rd; // Obtain a random number from hardware
  std::mt19937 gen(rd()); // Seed the generator
  std::uniform_int_distribution distr(0, 255); // Define the range;
  std::string blob;
  for (int i = 0; i < 128; ++i) {
    auto randomChar = static_cast<unsigned char>(distr(gen));
    blob.push_back(randomChar);
  }
  hd::type::parsed_packet _packet;
  _packet.HasContent = true;
  _packet.protocol = 6;
  _packet.mTsSec = 1723235588;
  _packet.mTSuSec = 23641;
  _packet.mKey = "127.0.0.1_192.168.8.124_22_4522_TCP";
  _packet.mBlobData = blob;
  std::uniform_int_distribution dist(60, 1255); // Define the range;
  _packet.mCapLen = dist(gen);
  return _packet;
}

int main(int argc, char* argv[]) {
  hd::type::hd_flow _flow;
  _flow.count = 10;
  _flow.protocol = IPPROTO_TCP;
  _flow.flowId = "127.0.0.1_192.168.8.124_22_4522_TCP";

  std::vector<hd::type::parsed_packet> _packets;
  _packets.reserve(10);
  for (int i = 0; i < 10; ++i) {
    _packets.emplace_back(genrate());
  }
  auto t = encode(_flow);
  print(t);
  return 0;
}
