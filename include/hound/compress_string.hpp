//
// hound-torch / compress_string.hpp
// Created by brian on 2024-05-24.
//

#ifndef HOUND_TORCH_COMPRESS_STRING_HPP
#define HOUND_TORCH_COMPRESS_STRING_HPP

#include <string>
// #include <array>
// #include <zlib.h>
#include <zstd.h>
#include <stdexcept>

namespace zstd {
void static compress(const std::string& src, std::string& dst) {
  const size_t _dst_capacity = ZSTD_compressBound(src.size());
  dst.resize(_dst_capacity);
  const size_t c_size = ZSTD_compress(&dst[0], _dst_capacity, &src[0], src.size(), 12);
  dst.resize(c_size);
}

void static decompress(const std::string& src, std::string& dst) {
  const size_t d_buff_size = ZSTD_getFrameContentSize(&src[0], src.size());
  if (d_buff_size == ZSTD_CONTENTSIZE_ERROR or d_buff_size == ZSTD_CONTENTSIZE_UNKNOWN) [[unlikely]] {
    throw std::runtime_error("Unable to determine decompressed size.");
  }
  dst.assign(d_buff_size, '\0');
  const size_t d_size = ZSTD_decompress(&dst[0], d_buff_size, &src[0], src.size());
  dst.resize(d_size);
}
}//zstd
/*
namespace zlib {
static void compress(const std::string& str, std::string& out, int const level = Z_BEST_COMPRESSION) {
  z_stream zs{};
  if (deflateInit2(&zs, level, Z_DEFLATED, 31, 9, Z_DEFAULT_STRATEGY) != Z_OK) {
    throw std::runtime_error("deflateInit2 failed while compressing.");
  }
  zs.next_in = (Bytef*) str.data();
  zs.avail_in = str.size();
  int ret;
  std::array<char, 65536> outbuffer{};

  do {
    zs.next_out = reinterpret_cast<Bytef*>(outbuffer.data());
    zs.avail_out = sizeof outbuffer;
    ret = deflate(&zs, Z_FINISH);
    if (out.size() < zs.total_out) {
      out.append(outbuffer.data(), zs.total_out - out.size());
    }
  } while (ret == Z_OK);

  deflateEnd(&zs);

  if (ret != Z_STREAM_END) {
    std::ostringstream oss;
    oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
    throw std::runtime_error(oss.str());
  }
}

void static decompress(const std::string& input, std::string& out) {
  z_stream zs{};
  if (inflateInit2(&zs, 15 + 32) != Z_OK) {
    throw std::runtime_error("inflateInit2 failed while decompressing.");
  }
  zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
  zs.avail_in = input.size();

  int ret;
  char outbuffer[65536];
  do {
    zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
    zs.avail_out = sizeof(outbuffer);
    ret = inflate(&zs, Z_NO_FLUSH);
    if (out.size() < zs.total_out) {
      out.append(outbuffer, zs.total_out - out.size());
    }
    if (ret != Z_OK && ret != Z_STREAM_END) {
      std::ostringstream oss;
      oss << "Exception during zlib decompression: (" << ret << ") " << zs.msg;
      inflateEnd(&zs);
      throw std::runtime_error(oss.str());
    }
  } while (ret != Z_STREAM_END);

  inflateEnd(&zs);
}

}//zlib
*/

#endif //HOUND_TORCH_COMPRESS_STRING_HPP
