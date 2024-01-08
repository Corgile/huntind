// //
// // Created by brian on 11/27/23.
// //
//
// #ifndef HOUND_CORE_HPP
// #define HOUND_CORE_HPP
//
// #include <hound/type/stride_t.hpp>
// #include <hound/type/byte_array.hpp>
//
// namespace hd::core {
// using namespace hd::type;
// using namespace hd::global;
//
// #ifndef BIT
//   #define BIT(x) ((x) << 3)
// #endif//BIT
//
// static void ConvertToBits(uint32_t const value, std::string& buffer) {
//   if (value == 0) {
//     buffer.append("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,");
//     return;
//   }
//   char buff[4];
//   for (int i = 31; i >= 0; --i) {
//     int const bit = value >> i & 0x1;
//     std::snprintf(buff, sizeof(buff), "%d,", bit);
//     buffer.append(buff);
//   }
// }
//
// static void ProcessStride_1(const int PadSize, const ByteArray& rawData, std::string& buffer) {
//   int i = 0;
//   int const _numField = BIT(rawData.byteLen) / 32;
//   int const _padField = BIT(PadSize) / 32;
//   uint32_t const* arr = reinterpret_cast<uint32_t*>(rawData.data);
//   for (; i < _numField; ++i) {
//     ConvertToBits(arr[i], buffer);
//   }
//   for (; i < _padField; ++i) {
//     ConvertToBits(opt.fill_bit, buffer);
//   }
// }
//
// static void ProcessStride_8(const int PadSize, const ByteArray& rawData, std::string& buffer) {
//   int i = 0;
//   uint8_t const* arr = rawData.data;
//   int const _numField = rawData.byteLen;
//   int const _padField = PadSize;
//   for (; i < _numField; ++i) {
//     buffer.append(std::to_string(arr[i])).append(",");
//   }
//   for (; i < _padField; ++i) {
//     buffer.append(fillBit);
//   }
// }
//
// static void ProcessStride_16(const int PadSize, const ByteArray& rawData, std::string& buffer) {
//   int i = 0;
//   int32_t const _numField{BIT(rawData.byteLen) / 16};
//   int32_t const _paddedField{BIT(PadSize) / 16};
//   auto const* arr = reinterpret_cast<uint16_t*>(rawData.data);
//   for (; i < _numField; ++i) {
//     buffer.append(std::to_string(arr[i])).append(",");
//   }
//   for (; i < _paddedField; ++i) {
//     buffer.append(fillBit);
//   }
// }
//
// static void ProcessStride_32(const int PadSize, const ByteArray& rawData, std::string& buffer) {
//   int i = 0;
//   int const _numField = BIT(rawData.byteLen) / 32;
//   int const _padField = BIT(PadSize) / 32;
//   uint32_t const* arr = reinterpret_cast<uint32_t*>(rawData.data);
//   for (; i < _numField; ++i) {
//     buffer.append(std::to_string(arr[i])).append(",");
//   }
//   for (; i < _padField; ++i) {
//     buffer.append(fillBit);
//   }
// }
//
// static void ProcessStride_64(const int PadSize, const ByteArray& rawData, std::string& buffer) {
//   int i = 0;
//   int const _numField = rawData.byteLen / sizeof(uint64_t);
//   int const _padField = BIT(PadSize) / 32;
//   uint64_t const* arr = reinterpret_cast<uint64_t*>(rawData.data);
//   for (; i < _numField; ++i) {
//     buffer.append(std::to_string(arr[i])).append(",");
//   }
//   for (; i < _padField; ++i) {
//     buffer.append(fillBit);
//   }
// }
//
// template<int32_t PadSize>
// static void ProcessByteArray(bool const condition, const ByteArray& rawData, std::string& buffer) {
//   if (not condition) return;
//   if (opt.stride == 1) ProcessStride_1(PadSize, rawData, buffer);
//   if (opt.stride == 8) ProcessStride_8(PadSize, rawData, buffer);
//   if (opt.stride == 16) ProcessStride_16(PadSize, rawData, buffer);
//   if (opt.stride == 32) ProcessStride_32(PadSize, rawData, buffer);
//   if (opt.stride == 64) ProcessStride_64(PadSize, rawData, buffer);
// }
//
// static void ProcessByteArray(bool const condition, const ByteArray& rawData, std::string& buffer) {
//   if (not condition) return;
//   if (opt.stride == 1) ProcessStride_1(opt.payload, rawData, buffer);
//   if (opt.stride == 8) ProcessStride_8(opt.payload, rawData, buffer);
//   if (opt.stride == 16) ProcessStride_16(opt.payload, rawData, buffer);
//   if (opt.stride == 32) ProcessStride_32(opt.payload, rawData, buffer);
//   if (opt.stride == 64) ProcessStride_64(opt.payload, rawData, buffer);
// }
//
//
// }//namespace hd::core
// #endif //HOUND_CORE_HPP
