//
// Created by brian on 3/13/24.
//
#include "hound/type/hd_flow.hpp"

hd::type::hd_flow::hd_flow(std::string _flowId, parsed_vector& _data) {
  protocol = static_cast<int>(_data[0].protocol);
  flowId = std::move(_flowId);
  _packet_list = std::move(_data);
  count = _packet_list.size();
}
