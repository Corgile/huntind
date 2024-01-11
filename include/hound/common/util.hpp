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
{"worker", required_argument, nullptr, 'J'},
{"filter", required_argument, nullptr, 'F'},
{"num", required_argument, nullptr, 'N'},
{"csv", required_argument, nullptr, 'C'},
{"pcap", required_argument, nullptr, 'P'},
{"out", required_argument, nullptr, 'O'},
/// no args
{"help", no_argument, nullptr, 'h'},
{nullptr, 0, nullptr, 0}
};
static char const* shortopts = "J:F:N:C:P:O:h";
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

static pcap_t* OpenDeadHandle(const capture_option& option, uint32_t& link_type) {
  using offline = pcap_t* (*)(const char*, char*);
  offline const open_offline{pcap_open_offline};
  if (not fs::exists(option.pcap)) {
    hd_line("无法打开文件 ", option.pcap);
    exit(EXIT_FAILURE);
  }
  auto const handle{open_offline(option.pcap.c_str(), ByteBuffer)};
  SetFilter(handle);
  link_type = pcap_datalink(handle);
  return handle;
}

static void Doc() {
  std::cout << "\t选项: " << shortopts << '\n';
  std::cout
    << "\t-J, --worker=1           pcap_loop的callback线程数\n"
    << "\t    --filter=\"filter\"  参考： man7 pcap filter \n"
    << "\t-P, --pcap=/path/pcap    数据集pcap\n"
    << "\t    --csv                带标签数据集的csv文件路径\n"
    << "\t    --num                读取pcap文件num条\n"
    << "\t    --out                生成的.data、.label文件路径\n"
    << "\t-h, --help               文档\n"
    << std::endl;
}

static void ParseOptions(capture_option& arguments, int argc, char* argv[]) {
  int longind = 0, option, j;
  opterr = 0;
  while ((option = getopt_long(argc, argv, shortopts, longopts, &longind)) not_eq -1) {
    switch (option) {
    // @formatter:off
    case 'F': arguments.filter.assign(optarg); break;
    case 'C': arguments.csv.assign(optarg); break;
    case 'P': arguments.pcap.assign(optarg); break;
    case 'O': arguments.out.assign(optarg); break;
    case 'N': arguments.num = std::stoi(optarg); break;
    // @formatter:on
    case 'J': j = std::stoi(optarg);
      if (j < 1) {
        hd_line("worker 需 >= 1");
        exit(EXIT_FAILURE);
      }
      arguments.worker = j;
      break;
    case '?':
      hd_line("选项 ", '-', std::to_string((char)optopt), ":" RED(" 有误"));
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
