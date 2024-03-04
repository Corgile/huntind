//
// Created by brian on 11/22/23.
//

#ifndef HOUND_UTILS_HPP
#define HOUND_UTILS_HPP

#include <getopt.h>
#include <memory>
#include <string>

#include <hound/common/macro.hpp>
#include <hound/common/global.hpp>
#include <hound/type/capture_option.hpp>
#include <hound/type/deleters.hpp>

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
  {"num", required_argument, nullptr, 'N'},
/// min packets
  {"min-packets", required_argument, nullptr, 'L'},
/// max packets
  {"max-packets", required_argument, nullptr, 'R'},
/// packet timeout seconds(to determine whether to send)
  {"timeout", required_argument, nullptr, 'E'},
  {"threads", required_argument, nullptr, 'r'},
  {"port", required_argument, nullptr, 'P'},
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
static char const* shortopts = "J:P:W:F:f:N:E:D:S:L:R:p:CTVhIm:";
#pragma endregion ShortAndLongOptions //@formatter:on

inline void SetFilter(pcap_handle_t const& handle) {
  if (opt.filter.empty() or handle == nullptr) { return; }
  constexpr bpf_u_int32 net{0};
  bpf_program fp{};
  hd_debug("包过滤表达式: ", opt.filter);
  if (pcap_compile(handle.get(), &fp, opt.filter.c_str(), 0, net) == -1) {
    std::printf("%s%s\n", RED("解析 Filter 失败: "), pcap_geterr(handle.get()));
    exit(EXIT_FAILURE);
  }
  if (pcap_setfilter(handle.get(), &fp) == -1) {
    std::printf("%s%s\n", RED("设置 Filter 失败: "), pcap_geterr(handle.get()));
    exit(EXIT_FAILURE);
  }
  pcap_freecode(&fp);
}

static void OpenLiveHandle(capture_option& option, pcap_handle_t& handle) {
  /* getFlowId device */
  if (option.device.empty()) {
    pcap_if_t* l;
    if (int32_t const rv{pcap_findalldevs(&l, ByteBuffer)}; rv == -1) {
      std::printf("%s%s\n", RED("找不到默认网卡设备: "), ByteBuffer);
      exit(EXIT_FAILURE);
    }
    option.device = l->name;
    pcap_freealldevs(l);
  }
  hd_debug("网卡: ", option.device);
  /* open device */
  handle.reset(pcap_open_live(option.device.c_str(), BUFSIZ, 1, 1000, ByteBuffer));
  if (handle == nullptr) {
    std::printf("%s%s\n", RED("监听网卡设备失败: "), ByteBuffer);
    exit(EXIT_FAILURE);
  }
  SetFilter(handle);
  pcap_set_promisc(handle.get(), 1);
  pcap_set_buffer_size(handle.get(), 25 << 22);
  // link_type = pcap_datalink(handle);
  // hd_debug(link_type);
}

inline void Doc() {
  std::cout << "\t用法: \n";
  std::cout
    << "\t-J, --workers=1               处理流量包的线程数 (默认 1)\n"
    << "\t-F, --filter=\"filter\"         pcap filter (https://linux.die.net/man/7/pcap-filter)\n"
    << "                              " RED("\t非常重要,必须设置并排除rpc server和client之间的流量,比如 \"not port 9001\"\n")
    << "\t-f, --fill=0                  空字节填充值 (默认 0)\n"
    << "\t-D, --duration=-1             D秒后结束抓包  (默认 -1, non-stop)\n"
    << "\t-N, --num=-1                  指定抓包的数量 (默认 -1, non-stop)\n"
    << "\t-E, --timeout=20              flow超时时间(新到达的packet距离上一个packet的时间) (默认 20)\n"
    << "\t    --thread=10               rpc服务端线程数 (默认 10)\n"
    << "\t    --port=9001               rpc服务端的通信端口 （默认 9001）\n"
    << "\t-L, --min-packets=10          合并成流/json的时候，指定流的最 小 packet数量 (默认 10)\n"
    << "\t-R, --max-packets=100         合并成流/json的时候，指定流的最 大 packet数量 (默认 100)\n"
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

static void ParseOptions(capture_option& arg, int argc, char* argv[]) {
  int longind = 0, option, j;
  opterr = 0;
  while ((option = getopt_long(argc, argv, shortopts, longopts, &longind)) not_eq -1) {
    switch (option) {
    case 'd': arg.device = optarg;
      break;
    case 'D': arg.duration = std::stoi(optarg);
      break;
    case 'C': arg.include_pktlen = true;
      break;
    case 'F': arg.filter = optarg;
      break;
    case 'f': arg.fill_bit = std::stoi(optarg);
      break;
    case 'r': arg.threads = std::stoi(optarg);
      break;
    case 'P': arg.port = std::stoi(optarg);
      break;
    case 'N': arg.num_packets = std::stoi(optarg);
      break;
    case 'p': arg.payload = std::stoi(optarg);
      break;
    case 'L': arg.min_packets = std::stoi(optarg);
      break;
    case 'R': arg.max_packets = std::stoi(optarg);
      break;
    case 'E': arg.flowTimeout = std::stoi(optarg);
      break;
    case 'T': arg.include_ts = true;
      break;
    case 'V': arg.verbose = true;
      break;
    case 'm': arg.separator.assign(optarg);
      std::sprintf(arg.format, "%s%s", "%ld", optarg);
      break;
    case 'I': arg.include_5tpl = true;
      break;
    case 'J': j = std::stoi(optarg);
      if (j < 1) {
        std::printf("%s\n", RED("worker 必须 >= 1"));
        exit(EXIT_FAILURE);
      }
      arg.workers = j;
      break;
    case 'S': arg.stride = std::stoi(optarg);
      if (arg.stride & arg.stride - 1 or arg.stride == 0) {
        std::printf("%s%s\n", RED("-S,  --stride 只能是1,2,4,8,16,32,64, 现在是"), optarg);
        exit(EXIT_FAILURE);
      }
      break;
    case 'W': arg.write_file = true;
      arg.output_file = optarg;
      if (optarg == nullptr or arg.output_file.empty()) {
        std::printf("%s\n", RED("-W, --write 缺少值"));
        exit(EXIT_FAILURE);
      }
      break;
    case '?': std::printf("选项 -%c%s\n", optopt, RED(" 有语法错误"));
      std::printf("%s\n", "使用 -h, --help 查看使用方法");
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
