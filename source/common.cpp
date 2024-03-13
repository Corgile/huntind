//
// Created by brian on 3/13/24.
//
#include <hound/common/core.hpp>
#include <hound/type/parsed_data.hpp>
#include <hound/common/global.hpp>

using namespace hd::type;
using namespace hd::global;

[[maybe_unused]]
void hd::core::util::fillCsvBuffer(ParsedData const& data, std::string& buffer) {
  //@formatter:off
  if (opt.include_5tpl)   buffer.append(data.m5Tuple).append(opt.separator);
  if (opt.include_pktlen) buffer.append(data.mCapLen).append(opt.separator);
  if (opt.include_ts)     buffer.append(data.mTimestamp).append(opt.separator);
  //@formatter:on
  fillRawBitVec(data, buffer);
}

void hd::core::util::fillRawBitVec(ParsedData const& data, std::string& buffer) {
  using namespace global;
  core::util::fill<IP4_PADSIZE>(true, data.mIP4Head, buffer);
  core::util::fill<TCP_PADSIZE>(true, data.mTcpHead, buffer);
  core::util::fill<UDP_PADSIZE>(true, data.mUdpHead, buffer);
  fill(opt.payload > 0, data.mPayload, buffer);
  buffer.pop_back();
}

void hd::core::util::_fill(int const width, int const _exceptedBytes, std::string_view const raw, std::string& refout) {
  int i = 0;
  auto const p = reinterpret_cast<uint64_t const*>(raw.data());
  uint64_t const n = log2(width);
  uint64_t const s = log2(64 >> n);
  uint64_t const r = (64 >> n) - 1;
  uint64_t const f = get_ff(width);

  char buffer[22];
  for (; i < raw.length() << 3 >> n; ++i) {
    const uint64_t w = (i & r) << n;
    // invalid read
    const uint64_t _val = (f << w & p[i >> s]) >> w; //45 00   05 dc a9 93   20 00
    std::sprintf(buffer, opt.format, _val);
    refout.append(buffer);
  }
  for (; i < _exceptedBytes << 3 >> n; ++i) {
    refout.append(fillBit);
  }
}

uint64_t hd::core::util::get_ff(int const width) {
  uint64_t buff = 1;
  for (int i = 0; i < width - 1; ++i) {
    buff <<= 1;
    buff += 1;
  }
  // buff <<= 64 - width;
  return buff;
}

uint64_t hd::core::util::log2(int v) {
  int n = 0;
  while (v > 1) {
    v >>= 1;
    n++;
  }
  return n;
}
