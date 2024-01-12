//
// Created by brian on 12/27/23.
//

#ifndef HOUND_CORE_FUNC_HPP
#define HOUND_CORE_FUNC_HPP

#include <string>
#include <hound/type/byte_array.hpp>
#include <hound/common/global.hpp>

namespace hd::core {
using namespace hd::type;
using namespace hd::global;

class util {
public:
  template <int32_t PadBytes = -1>
  static void fill(bool const condition, const ByteArray& rawData, std::string& buffer) {
    if (not condition) return;
    if constexpr (PadBytes == -1) { // payload
      _fill(opt.stride, opt.payload, rawData, buffer);
    } else _fill(opt.stride, PadBytes, rawData, buffer);
  }

private:
  static uint64_t log2(int v) {
    int n = 0;
    while (v > 1) {
      v >>= 1;
      n++;
    }
    return n;
  }

  static uint64_t get_ff(const int width) {
    uint64_t buff = 1;
    for (int i = 0; i < width - 1; ++i) {
      buff <<= 1;
      buff += 1;
    }
    // buff <<= 64 - width;
    return buff;
  }

  static void _fill(int const width, int const _exceptedBytes, const ByteArray& raw, std::string& refout) {
    int i = 0;
    uint64_t const n = log2(width);
    uint64_t const s = log2(64 >> n);
    uint64_t const r = (64 >> n) - 1;
    uint64_t const f = get_ff(width);

    char buffer[22];
    for (; i < raw.byteLen << 3 >> n; ++i) {
      const uint64_t w = (i & r) << n;
      const uint64_t _val = (f << w & raw.data[i >> s]) >> w;//45 00   05 dc a9 93   20 00
      std::sprintf(buffer, opt.format, _val);
      refout.append(buffer);
    }
    for (; i < _exceptedBytes << 3 >> n; ++i) {
      refout.append(fillBit);
    }
  }
};
}// namespace hd::core

#endif // HOUND_CORE_FUNC_HPP
