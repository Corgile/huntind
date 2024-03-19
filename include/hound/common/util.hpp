//
// Created by brian on 11/22/23.
//

#ifndef HOUND_UTILS_HPP
#define HOUND_UTILS_HPP

#include <getopt.h>
#include <string>
#include <hound/common/macro.hpp>
#include <hound/common/global.hpp>
#include <hound/type/capture_option.hpp>
#include <hound/type/deleters.hpp>
#include <hound/type/parsed_data.hpp>
#include <hound/type/hd_flow.hpp>

namespace hd::util {
using namespace hd::global;
using namespace hd::type;
static char ByteBuffer[PCAP_ERRBUF_SIZE];

#pragma region ShortAndLongOptions
inline option longopts[] = {
  /// specify which network interface to capture
  {"device", required_argument, nullptr, 'd'},
  {"workers", required_argument, nullptr, 'J'},
  {"duration", required_argument, nullptr, 'D'},
  /// custom filter for libpcap
  {"filter", required_argument, nullptr, 'F'},
  {"fill", required_argument, nullptr, 'f'},
  {"num", required_argument, nullptr, 'N'},
  /// min packets
  {"min", required_argument, nullptr, 'L'},
  /// max packets
  {"max", required_argument, nullptr, 'R'},
  /// packet timeout seconds(to determine whether to send)
  {"timeout", required_argument, nullptr, 'E'},
  {"kafka", required_argument, nullptr, 'K'},
  {"sep", required_argument, nullptr, 'm'},
  {"index", required_argument, nullptr, 'I'},
  /// num of bits to convert as an integer
  {"stride", required_argument, nullptr, 'S'},
  /// dump output into a csv_path file
  {"write", required_argument, nullptr, 'W'},
  {"payload", required_argument, nullptr, 'p'},
  /// no argument
#if defined(HD_FUTURE_SUPPORT)
  {"radiotap",    no_argument,       nullptr, 'r'},
  {"wlan",        no_argument,       nullptr, 'w'},
  {"eth",         no_argument,       nullptr, 'e'},
  {"ipv6",        no_argument,       nullptr, '6'},
  {"icmp",        no_argument,       nullptr, 'i'},
  {"ipv4",        no_argument,       nullptr, '4'},
  {"tcp",         no_argument,       nullptr, 't'},
  {"udp",         no_argument,       nullptr, 'u'},
#endif
  {"help", no_argument, nullptr, 'h'},
  {"timestamp", no_argument, nullptr, 'T'},
  {"caplen", no_argument, nullptr, 'C'},
  {"verbose", no_argument, nullptr, 'V'},
  {nullptr, 0, nullptr, 0}
};
static char const* shortopts = "J:P:W:F:f:N:E:K:D:S:L:R:p:CTVhIm:";
#pragma endregion ShortAndLongOptions //@formatter:on

void SetFilter(pcap_handle_t& handle);

void OpenLiveHandle(capture_option& option, pcap_handle_t& handle);

void Doc();

void ParseOptions(capture_option& arg, int argc, char* argv[]);

bool IsFlowReady(packet_list const& existing, hd_packet const& _new);

namespace detail {

using namespace hd::type;

using packet_list = std::vector<hd_packet>;

bool _isTimeout(packet_list const& existing, hd_packet const& _new);

bool _isTimeout(packet_list const& existing);

bool _checkLength(packet_list const& existing);

template<typename TimeUnit = std::chrono::seconds>
static long timestampNow() {
  auto const now = std::chrono::system_clock::now();
  auto const duration = now.time_since_epoch();
  return std::chrono::duration_cast<TimeUnit>(duration).count();
}

}
} // namespace hd::util
#endif //HOUND_UTILS_HPP
