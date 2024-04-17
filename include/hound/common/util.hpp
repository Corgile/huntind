//
// Created by brian on 11/22/23.
//

#ifndef HOUND_UTILS_HPP
#define HOUND_UTILS_HPP

#include <getopt.h>
#include <hound/common/macro.hpp>
#include <hound/common/global.hpp>
#include <hound/type/capture_option.hpp>
#include <hound/type/deleters.hpp>
#include <hound/type/parsed_packet.hpp>

namespace hd::util {
using namespace hd::global;
using namespace hd::type;
static char ByteBuffer[PCAP_ERRBUF_SIZE];

#pragma region ShortAndLongOptions
inline option longopts[] = {
  // @format:off
  /// specify which network interface to capture
  {"device",    required_argument, nullptr, 'd'},
  {"workers",   required_argument, nullptr, 'J'},
  {"duration",  required_argument, nullptr, 'D'},
  /// custom filter for libpcap
  {"filter",    required_argument, nullptr, 'F'},
  {"fill",      required_argument, nullptr, 'f'},
  {"cuda",      required_argument, nullptr, 'c'},
  {"num",       required_argument, nullptr, 'N'},
  /// min packets
  {"min",       required_argument, nullptr, 'L'},
  /// max packets
  {"max",       required_argument, nullptr, 'R'},
  /// packet timeout seconds(to determine whether to send)
  {"timeout",   required_argument, nullptr, 'E'},
  // {"kafka",     required_argument, nullptr, 'K'},
  {"sep",       required_argument, nullptr, 'm'},
  {"brokers",   required_argument, nullptr, 'B'},
  {"pool",      required_argument, nullptr, 'b'},
  {"partition", required_argument, nullptr, 'x'},
  {"topic",     required_argument, nullptr, 'y'},
  {"model",     required_argument, nullptr, 'M'},
  {"index",     required_argument, nullptr, 'I'},
  /// num of bits to convert as an integer
  {"stride",    required_argument, nullptr, 'S'},
  /// dump output into a csv_path file
  {"write",     required_argument, nullptr, 'W'},
  {"payload",   required_argument, nullptr, 'p'},
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
  {"help",        no_argument,     nullptr, 'h'},
  {"timestamp",   no_argument,     nullptr, 'T'},
  {"caplen",      no_argument,     nullptr, 'C'},
  {"verbose",     no_argument,     nullptr, 'V'},
  {nullptr,       0,               nullptr, 0}
};

inline std::unordered_map<int, std::string_view> signal_msgs {
  {23, "SIGURG: Urgent data is available at a socket."},
  {19, "SIGSTOP: Stop, unblockable."},
  {20, "SIGTSTP: Keyboard stop."},
  {18, "SIGCONT: Continue."},
  {17, "SIGCHLD: Child terminated or stopped."},
  {21, "SIGTTIN: Background read from control terminal."},
  {22, "SIGTTOU: Background write to control terminal."},
  {29, "SIGPOLL: Pollable event occurred (System V)."},
  {25, "SIGXFSZ: File size limit exceeded."},
  {24, "SIGXCPU: CPU time limit exceeded."},
  {26, "SIGVTALR: Virtual timer expired."},
  {27, "SIGPROF: Profiling timer expired."},
  {16,"SIGSTKFLT: Stack fault (obsolete)."},
  {30,"SIGPWR: Power failure imminent."},
  { 7,"SIGBUS: Bus error."},
  {31,"SIGSYS: Bad system call."},
  {	2, "SIGINT: Interactive attention signal."},
  {	4, "SIGILL: Illegal instruction."},
  {	6, "SIGABRT: Abnormal termination."},
  {	8, "SIGFPE: Erroneous arithmetic operation."},
  {11, "SIGSEGV: Invalid access to storage."},
  {15, "SIGTERM: Termination request."},
  {	1, "SIGHUP: Hangup."},
  {	3, "SIGQUIT: Quit."},
  {	5, "SIGTRAP: Trace/breakpoint trap."},
  {	9, "SIGKILL: Killed."},
  {13, "SIGPIPE: Broken pipe."},
  {14, "SIGALRM: Alarm clock."},
};
static auto shortopts = "c:J:P:W:F:f:N:E:D:S:L:R:p:CTVhIM:m:B:b:x:y:";
// "K:"

#pragma endregion ShortAndLongOptions //@format:on

void SetFilter(pcap_handle_t& handle);

void OpenLiveHandle(capture_option& option, pcap_handle_t& handle);

void Doc();

void ParseOptions(capture_option& arg, int argc, char* argv[]);

bool IsFlowReady(parsed_vector const& existing, parsed_packet const& _new);

namespace detail {
using namespace hd::type;

bool _isTimeout(parsed_vector const& existing, parsed_packet const& _new);

bool _isTimeout(parsed_vector const& existing);

bool _checkLength(parsed_vector const& existing);

template <typename TimeUnit = std::chrono::seconds>
static long timestampNow() {
  auto const now = std::chrono::system_clock::now();
  auto const duration = now.time_since_epoch();
  return std::chrono::duration_cast<TimeUnit>(duration).count();
}
}

namespace literals {
constexpr unsigned long long operator""_B(unsigned long long bytes) {
  return bytes;
}

constexpr unsigned long long operator""_KB(unsigned long long kilobytes) {
  return kilobytes << 10;
}

constexpr unsigned long long operator""_MB(unsigned long long megabytes) {
  return megabytes << 20;
}

constexpr unsigned long long operator""_GB(unsigned long long gigabytes) {
  return gigabytes << 30;
}
}
} // namespace hd::util
#endif //HOUND_UTILS_HPP
