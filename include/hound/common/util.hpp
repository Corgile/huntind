//
// Created by brian on 11/22/23.
//

#ifndef HOUND_UTILS_HPP
#define HOUND_UTILS_HPP

#include <filesystem>
#include <getopt.h>
#include <memory>
#include <string>
#include <hound/common/macro.hpp>
#include <hound/common/global.hpp>
#include <hound/type/capture_option.hpp>
#include <hound/type/parsed_data.hpp>

namespace hd::util {
namespace fs = std::filesystem;
using namespace hd::global;
using namespace hd::type;
inline char ByteBuffer[PCAP_ERRBUF_SIZE];

#pragma region ShortAndLongOptions
static option longopts[] = {
/// specify which network interface to capture
{"device", required_argument, nullptr, 'd'},
{"workers", required_argument, nullptr, 'J'},
{"duration", required_argument, nullptr, 'D'},
/// custom filter for libpcap
{"filter", required_argument, nullptr, 'F'},
{"fill", required_argument, nullptr, 'f'},
{"num-packets", required_argument, nullptr, 'N'},
/// min packets
{"min", required_argument, nullptr, 'L'},
/// max packets
{"max", required_argument, nullptr, 'R'},
/// packet timeout seconds(to determine whether to send)
{"interval", required_argument, nullptr, 'E'},
{"kafka", required_argument, nullptr, 'K'},
/// pcap file path, required when processing a pcapng file.
{"pcap-file", required_argument, nullptr, 'P'},
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
static char const* shortopts = "J:P:W:F:f:N:S:L:R:p:CTVhIm:";
#pragma endregion ShortAndLongOptions //@formatter:on

static void SetFilter(pcap_t* handle) {
  if (opt.filter.empty() or handle == nullptr) { return; }
  constexpr bpf_u_int32 net{0};
  bpf_program fp{};
  hd_debug(opt.filter);
  if (pcap_compile(handle, &fp, opt.filter.c_str(), 0, net) == -1) {
    hd_line("解析 Filter 失败: ", opt.filter, "\n", pcap_geterr(handle));
    exit(EXIT_FAILURE);
  }
  if (pcap_setfilter(handle, &fp) == -1) {
    hd_line("设置 Filter 失败: ", pcap_geterr(handle));
    exit(EXIT_FAILURE);
  }
}

static pcap_t* OpenLiveHandle(capture_option& option) {
  /* getFlowId device */
  if (option.device.empty()) {
    pcap_if_t* l;
    int32_t const rv{pcap_findalldevs(&l, ByteBuffer)};
    if (rv == -1) {
      hd_line("找不到默认网卡设备", ByteBuffer);
      exit(EXIT_FAILURE);
    }
    option.device = l->name;
    pcap_freealldevs(l);
  }
  hd_debug(option.device);
  /* open device */
  auto const handle{pcap_open_live(option.device.c_str(), BUFSIZ, 1, 1000, ByteBuffer)};
  if (handle == nullptr) {
    hd_line("监听网卡设备失败: ", ByteBuffer);
    exit(EXIT_FAILURE);
  }
  SetFilter(handle);
  pcap_set_promisc(handle, 1);
  pcap_set_buffer_size(handle, 25 << 22);
  // link_type = pcap_datalink(handle);
  // hd_debug(link_type);
  return handle;
}

static pcap_t* OpenDeadHandle(const capture_option& option, uint32_t& link_type) {
  using offline = pcap_t* (*)(const char*, char*);
  offline const open_offline{pcap_open_offline};
  if (not fs::exists(option.pcap_file)) {
    hd_line("无法打开文件 ", option.pcap_file);
    exit(EXIT_FAILURE);
  }
  auto const handle{open_offline(option.pcap_file.c_str(), ByteBuffer)};
  SetFilter(handle);
  link_type = pcap_datalink(handle);
  return handle;
}

static void Doc() {
  std::cout << "\t选项: " << shortopts << '\n';
  std::cout
    << "\t-J, --workers=1               处理流量包的线程数 (默认 1)\n"
    << "\t-F, --filter=\"filter\"         pcap filter (https://linux.die.net/man/7/pcap-filter)\n"
    << "                              " RED("\t非常重要,必须设置并排除镜像流量服务器和kafka集群之间的流量,比如 \"not port 9092\"\n")
    << "\t-f, --fill=0                  空字节填充值 (默认 0)\n"
    << "\t-L, --min-packets=10          合并成流/json的时候，指定流的最 小 packet数量 (默认 10)\n"
    << "\t-R, --max-packets=100         合并成流/json的时候，指定流的最 大 packet数量 (默认 100)\n"
    << "\t-P, --pcap-file=/path/pcap    pcap文件路径, 处理离线 pcap,pcapng 文件\n"
    << "\t-W, --write=/path/out         输出到文件, 需指定输出文件路径\n"
    << "\t-S, --stride=8                将 S 位二进制串转换为 uint 数值 (默认 8)\n"
    << "\t-p, --payload=0               包含 n 字节的 payload (默认 0)\n"
    << "\t    --sep=,                   csv列分隔符 (默认 ,)\n"
    << "\t-----------------" CYAN("以下选项不需要传入值")"----------------------------\n"
    << "\t-T, --timestamp               包含时间戳(秒,毫秒) (默认 不包含)\n"
    << "\t-C, --caplen                  包含报文长度 (默认 不包含)\n"
    << "\t-I, --index                   包含五元组 (默认 不包含)\n"
    << "\t-h, --help                    用法帮助\n"
    << std::endl;
}

static void ParseOptions(capture_option& arguments, int argc, char* argv[]) {
  int longind = 0, option, j;
  opterr = 0;
  while ((option = getopt_long(argc, argv, shortopts, longopts, &longind)) not_eq -1) {
    switch (option) {
    case 'd': arguments.device = optarg;
      break;
    case 'C': arguments.include_pktlen = true;
      break;
    case 'F': arguments.filter = optarg;
      break;
    case 'f': arguments.fill_bit = std::stoi(optarg);
      break;
    case 'N': arguments.num_packets = std::stoi(optarg);
      break;
    case 'p': arguments.payload = std::stoi(optarg);
      break;
    case 'L': arguments.min_packets = std::stoi(optarg);
      break;
    case 'R': arguments.max_packets = std::stoi(optarg);
      break;
    case 'E': arguments.packetTimeout = std::stoi(optarg);
      break;
    case 'T': arguments.include_ts = true;
      break;
    case 'V': arguments.verbose = true;
      break;
    case 'm': arguments.separator = optarg;
      std::sprintf(arguments.format, "%s%s", "%ld", optarg);
      break;
    case 'I': arguments.include_5tpl = true;
      break;
    case 'J': j = std::stoi(optarg);
      if (j < 1) {
        hd_line("worker 必须 >= 1");
        exit(EXIT_FAILURE);
      }
      arguments.workers = j;
      break;
    case 'S': arguments.stride = std::stoi(optarg);
      if (arguments.stride & arguments.stride - 1 or arguments.stride == 0) {
        hd_line("-S,  --stride 只能是1,2,4,8,16,32,64, 现在是: ", arguments.stride);
        exit(EXIT_FAILURE);
      }
      break;
    case 'W': arguments.write_file = true;
      arguments.output_file = optarg;
      if (optarg == nullptr or arguments.output_file.empty()) {
        hd_line("-W, --write 缺少值");
        exit(EXIT_FAILURE);
      }
      break;
    case 'P': arguments.pcap_file = optarg;
      if (arguments.pcap_file.empty()) {
        hd_line("-P, --pcap-file 缺少值");
        exit(EXIT_FAILURE);
      }
      break;
    case '?':
      hd_line("选项 ", '-', static_cast<char>(optopt), ":" RED(" 语法错误"));
      hd_line("使用 -h, --help 查看使用方法");
      exit(EXIT_FAILURE);
    case 'h': Doc();
      exit(EXIT_SUCCESS);
    default: break;
    }
  }
}

template <typename T>
static int min(T _a, T _b) {
  return _a < _b ? _a : _b;
}
} // namespace hd::util
#endif //HOUND_UTILS_HPP
