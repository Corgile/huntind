// //
// // Created by brian on 12/27/23.
// //
//
// #ifndef HOUND_CORE_FUNC_HPP
// #define HOUND_CORE_FUNC_HPP
//
// #include <string>
// #include <hound/common/macro.hpp>
// #include <hound/type/parsed_packet.hpp>
//
// namespace hd::util {
// using namespace hd::type;
//
// [[maybe_unused]]
// inline void fillRawBitVec(parsed_packet const& data, std::vector<uint32_t>& buffer);
//
// namespace detail {
// using namespace hd::global;
//
// inline void _fill(int const, int const, const std::string_view, std::vector<uint32_t>&);
//
// template<int32_t PadBytes = -1>
// static void fill(bool const condition, const std::string_view rawData, std::vector<uint32_t>& buffer) {
//   if (not condition) return;
//   if constexpr (PadBytes == -1) {
//     detail::_fill(opt.stride, opt.payload, rawData, buffer);
//   } else detail::_fill(opt.stride, PadBytes, rawData, buffer);
// }
//
// inline static uint64_t log2(int v);
//
// inline static uint64_t get_ff(const int width);
// }// detail
// }//hd::util
//
// #endif // HOUND_CORE_FUNC_HPP
