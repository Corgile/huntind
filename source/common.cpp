// //
// // Created by brian on 3/13/24.
// //
// #include "hound/common/core.hpp"
// #include "hound/type/parsed_packet.hpp"
// #include "hound/common/global.hpp"
//
// using namespace hd::global;
//
// [[maybe_unused]] void hd::util::fillRawBitVec(parsed_packet const& data, std::vector<uint32_t>& buffer) {
//   buffer.reserve(IP4_PADSIZE + TCP_PADSIZE + UDP_PADSIZE + opt.payload);
//   using namespace global;
//   util::detail::fill<IP4_PADSIZE>(true, data.mIP4Head, buffer);
//   util::detail::fill<TCP_PADSIZE>(true, data.mTcpHead, buffer);
//   util::detail::fill<UDP_PADSIZE>(true, data.mUdpHead, buffer);
//   util::detail::fill(opt.payload > 0,   data.mPayload, buffer);
// }
//
// void hd::util::detail::_fill(int const width,
//                              int const _excepted,
//                              std::string_view const raw,
//                              std::vector<uint32_t>& refout) {
//   int i = 0;
//   auto const p = reinterpret_cast<uint64_t const*>(raw.data());
//   uint64_t const n = log2(width);
//   uint64_t const s = log2(64 >> n);
//   uint64_t const r = (64 >> n) - 1;
//   uint64_t const f = get_ff(width);
//
//   for (; i < raw.length() << 3 >> n; ++i) {
//     const uint64_t w = (i & r) << n;
//     //45 00   05 dc a9 93   20 00
//     refout.emplace_back((f << w & p[i >> s]) >> w);
//   }
//   for (; i < _excepted << 3 >> n; ++i) {
//     refout.emplace_back(opt.fill_bit);
//   }
// }
//
// uint64_t hd::util::detail::get_ff(int const width) {
//   uint64_t buff = 1;
//   for (int i = 0; i < width - 1; ++i) {
//     buff <<= 1;
//     buff += 1;
//   }
//   // buff <<= 64 - width;
//   return buff;
// }
//
// uint64_t hd::util::detail::log2(int v) {
//   int n = 0;
//   while (v > 1) {
//     v >>= 1;
//     n++;
//   }
//   return n;
// }
