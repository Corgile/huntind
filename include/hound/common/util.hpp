//
// Created by brian on 11/22/23.
//

#ifndef HOUND_UTILS_HPP
#define HOUND_UTILS_HPP

#include <filesystem>
#include <getopt.h>
#include <memory>
#include <hound/common/macro.hpp>
#include <hound/type/capture_option.hpp>
#include <hound/type/parsed_data.hpp>
#include <hound/type/deleters.hpp>

namespace hd::util::details {
using namespace hd::global;
using namespace hd::type;

#pragma region ShortAndLongOptions
static option longopts[] = {
{"workers", required_argument, nullptr, 'J'},
/// custom filter for libpcap
{"filter", required_argument, nullptr, 'F'},
{"fill", required_argument, nullptr, 'f'},
{"num-packets", required_argument, nullptr, 'N'},
/// min packets
{"min", required_argument, nullptr, 'L'},
/// max packets
{"max", required_argument, nullptr, 'R'},
/// packet timeout seconds(to determine whether to send)
{"timeout", required_argument, nullptr, 'E'},
/// pcap file path, required when processing a pcapng file.
{"pcap", required_argument, nullptr, 'P'},
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
{"unsign", no_argument, nullptr, 'U'},
{"verbose", no_argument, nullptr, 'V'},
{nullptr, 0, nullptr, 0}
};
static char const* shortopts = "hJ:P:W:F:f:N:S:L:R:p:CTUVIm:";
#pragma endregion ShortAndLongOptions //@formatter:on

static void SetFilter(pcap_t* handle) {
  if (opt.filter.empty() or handle == nullptr) { return; }
  constexpr bpf_u_int32 net{0};
  bpf_program fp{0,nullptr};
  hd_debug(opt.filter);
  if (pcap_compile(handle, &fp, opt.filter.c_str(), 0, net) == -1) {
    hd_line("解析 Filter 失败: ", opt.filter, "\n", pcap_geterr(handle));
    exit(EXIT_FAILURE);
  }
  if (pcap_setfilter(handle, &fp) == -1) {
    hd_line("设置 Filter 失败: ", pcap_geterr(handle));
    exit(EXIT_FAILURE);
  }
  pcap_freecode(&fp);
}

static void Doc() {
  std::cout << "\t选项: " << shortopts << '\n';
  std::cout
    << "\t-J, --workers=1               处理流量包的线程数 (默认 1)\n"
    << "\t-F, --filter=\"filter\"         pcap filter (https://linux.die.net/man/7/pcap-filter)\n"
    << "                              " RED("\t非常重要,必须设置并排除镜像流量服务器和kafka集群之间的流量,比如 \"not port 9092\"\n")
    << "\t-f, --fill=0                  空字节填充值 (默认 0)\n"
    << "\t-E, --timeout=20              流超时时长 (默认 20秒)\n"
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
}

namespace hd::util {
namespace fs = std::filesystem;
using namespace hd::type;

static void OpenDeadHandle(const capture_option& option, pcap_handle_t& handle, uint32_t& link_type) {
  if (not fs::exists(option.pcap_file)) {
    hd_line("无法打开文件 ", option.pcap_file);
    exit(EXIT_FAILURE);
  }
  char err_buff[PCAP_ERRBUF_SIZE];
  handle.reset(pcap_open_offline(option.pcap_file.c_str(), err_buff));
  details::SetFilter(handle.get());
  link_type = pcap_datalink(handle.get());
}

static void ParseOptions(capture_option& arg, const int argc, char* argv[]) {
  int longind = 0, option, j;
  while ((option = getopt_long(argc, argv, details::shortopts, details::longopts, &longind)) not_eq -1) {
    switch (option) {
    case 'C': arg.include_pktlen = true;
      break;
    case 'F': arg.filter = optarg;
      break;
    case 'f': arg.fill_bit = std::stoi(optarg);
      break;
    case 'N': arg.num_packets = std::stoi(optarg);
      break;
    case 'p': arg.payload = std::stoi(optarg);
      break;
    case 'L': arg.min_packets = std::stoi(optarg);
      break;
    case 'R': arg.max_packets = std::stoi(optarg);
      break;
    case 'E': arg.packetTimeout = std::stoi(optarg);
      break;
    case 'T': arg.include_ts = true;
      break;
    case 'V': arg.verbose = true;
      break;
    case 'U': arg.unsign = true;
      arg.format[1] = 'u';
      break;
    case 'm': arg.separator = {optarg[0]};
      arg.format[2] = optarg[0];
      break;
    case 'I': arg.include_5tpl = true;
      break;
    case 'J': j = std::stoi(optarg);
      if (j < 1) {
        hd_line("worker 必须 >= 1");
        exit(EXIT_FAILURE);
      }
      arg.workers = j;
      break;
    case 'S': arg.stride = std::stoi(optarg);
      if (arg.stride & arg.stride - 1 or arg.stride == 0) {
        hd_line("-S,  --stride 只能是1,2,4,8,16,32,64, 现在是: ", arg.stride);
        exit(EXIT_FAILURE);
      }
      break;
    case 'W': arg.write_file = true;
      arg.output_file = optarg;
      if (optarg == nullptr or arg.output_file.empty()) {
        hd_line("-W, --write 缺少值");
        exit(EXIT_FAILURE);
      }
      break;
    case 'P': arg.pcap_file = optarg;
      if (arg.pcap_file.empty()) {
        hd_line("-P, --pcap-file 缺少值");
        exit(EXIT_FAILURE);
      }
      break;
    case '?':
      hd_line("选项 ", '-', static_cast<char>(optopt), ":" RED(" 语法错误"));
      hd_line("使用 -h, --help 查看使用方法");
      exit(EXIT_FAILURE);
    case 'h': details::Doc();
      exit(EXIT_SUCCESS);
    default: break;
    }
  }
}
} // namespace hd::util
#endif //HOUND_UTILS_HPP
