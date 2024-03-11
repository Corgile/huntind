//
// hound / deleters.hpp. 
// Created by brian on 2024-02-26.
//

#ifndef DELETERS_HPP
#define DELETERS_HPP

#include <memory>
#include <pcap/pcap.h>

struct pcap_deleter {
  // invalid write & read & free
  void operator()(pcap_t* pointer) const { pcap_close(pointer); }
};
using pcap_handle_t = std::unique_ptr<pcap_t, pcap_deleter>;

#endif //DELETERS_HPP
