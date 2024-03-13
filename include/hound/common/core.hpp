//
// Created by brian on 12/27/23.
//

#ifndef HOUND_CORE_FUNC_HPP
#define HOUND_CORE_FUNC_HPP

#include <string>
#include <hound/common/macro.hpp>
#include <hound/type/parsed_data.hpp>

namespace hd::core {
using namespace hd::type;
using namespace hd::global;

class util {
public:
  [[maybe_unused]]
  inline void fillCsvBuffer(ParsedData const& data, std::string& buffer);

  inline void fillRawBitVec(ParsedData const& data, std::string& buffer);

private:
  template<int32_t PadBytes = -1>
  static void fill(bool const condition, const std::string_view rawData, std::string& buffer) {
    if (not condition) return;
    if constexpr (PadBytes == -1) { // payload
      _fill(opt.stride, opt.payload, rawData, buffer);
    } else _fill(opt.stride, PadBytes, rawData, buffer);
  }

  inline static uint64_t log2(int v);

  inline static uint64_t get_ff(const int width);

  inline static void _fill(int const, int const, const std::string_view, std::string&);
};

} // namespace hd::core

#endif // HOUND_CORE_FUNC_HPP
