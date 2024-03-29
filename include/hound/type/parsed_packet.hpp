//
// Created by brian on 11/22/23.
//

#ifndef HOUND_PARSED_DATA_HPP
#define HOUND_PARSED_DATA_HPP

#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <hound/common/global.hpp>
#include <hound/type/raw_packet.hpp>

#include <ostream>
#include <string>
#include <ylt/easylog.hpp>

#define ETHERTYPE_IPV4 ETHERTYPE_IP

namespace hd::type {
struct parsed_packet;

using parsed_list = std::vector<parsed_packet>;

struct parsed_packet final {
  bool HasContent{true};
  uint16_t protocol;
  long mTsSec, mTSuSec, mCapLen;
  std::string mKey, mBlobData;

public:
  parsed_packet() = default;
  [[maybe_unused]]
  parsed_packet(raw_packet const& raw);
  /// @brief c'tor
  parsed_packet(parsed_packet const& copied);
  /// @brief m'tor
  parsed_packet(parsed_packet&& other) noexcept;
  /// swap
  parsed_packet& operator=(parsed_packet other);

  friend std::ostream& operator<<(std::ostream& os, parsed_packet const& packet) {
    os
      << "Key: " << packet.mKey
      << ", timestamp: " << packet.mTsSec << "." << packet.mTSuSec
      << ", raw data size: " << packet.mCapLen
      << ", blob data: [ " << packet.mBlobData.size() << " ] bytes";
    return os;
  }

private:
  [[nodiscard("do not discard"), maybe_unused]]
  bool processRawBytes(std::string_view _byteArr);
  [[nodiscard("do not discard"), maybe_unused]]
  bool processIPv4Packet(char const* _ip_bytes);

  template <typename HeaderType>
  void CopyPayloadToBlob(ip const* _ipv4,
                         char const* trans_header, const std::string_view suffix) {
    auto const* pHeaderType = reinterpret_cast<HeaderType const*>(trans_header);
    auto my_minmax = [](HeaderType const* pHeader)-> auto {
      return pHeader->source > pHeader->dest
               ? std::make_pair(ntohs(pHeader->source), ntohs(pHeader->dest))
               : std::make_pair(ntohs(pHeader->dest), ntohs(pHeader->source));
    };
    auto [_min, _max] = my_minmax(pHeaderType);
    mKey.append(std::to_string(_min)).append("_").append(std::to_string(_max)).append(suffix);
    size_t tl_hl = 8;
    if (protocol == IPPROTO_TCP) {
      tl_hl = reinterpret_cast<tcphdr const*>(trans_header)->doff * 4;
    }
    // payload
    const int available = ntohs(_ipv4->ip_len) - _ipv4->ip_hl * 4 - tl_hl;
    size_t const payload_len = std::min(std::max(available, 0), global::opt.payload);
    mBlobData.append(&trans_header[tl_hl], payload_len);
    int const padding = global::opt.payload + 128 - mBlobData.size();
    if (padding > 0) [[unlikely]] {
      mBlobData.append(padding, '\0');
    }
  }
};
} // hd

#endif //HOUND_PARSED_DATA_HPP
