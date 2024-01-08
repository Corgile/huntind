//
// Created by brian on 11/29/23.
//

#ifndef HOUND_VALUE_TRIPLE_HPP
#define HOUND_VALUE_TRIPLE_HPP

namespace hd::type {
struct PcapHeader {
  __time_t ts_sec;
  __suseconds_t ts_usec;
  bpf_u_int32 caplen;

  PcapHeader() = default;

  PcapHeader(__time_t const tsSec, __suseconds_t const tsUsec, bpf_u_int32 const capLen)
    : ts_sec(tsSec), ts_usec(tsUsec), caplen(capLen) {
  }
};
} // type

#endif //HOUND_VALUE_TRIPLE_HPP
