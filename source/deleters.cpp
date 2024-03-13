//
// Created by brian on 3/13/24.
//
#include <hound/type/deleters.hpp>

void pcap_deleter::operator()(pcap_t* pointer) const {
  pcap_close(pointer);
}
