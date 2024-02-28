#include <csignal>
#include <hound/common/util.hpp>
#include <hound/common/global.hpp>

#include <hound/parser/live_parser.hpp>

namespace hd::global {
type::capture_option opt;
std::string fillBit;
#if defined(BENCHMARK)
std::atomic<int32_t> packet_index = 0;
std::atomic<int32_t> num_captured_packet = 0;
std::atomic<int32_t> num_dropped_packets = 0;
std::atomic<int32_t> num_consumed_packet = 0;
std::atomic<int32_t> num_written_csv = 0;
#endif
}

static void quit_guard(const int max_, int& ctrlc) {
  auto const more = max_ - ++ctrlc;
  if (more > 0) {
    std::printf("%s%d%s\n", RED("如果没有立即停止就再按 "), more, RED(" 次 [Ctrl-C] 强制退出"));
  }
  if (ctrlc >= max_) {
    exit(EXIT_FAILURE);
  }
}

int main(const int argc, char* argv[]) {
  using namespace hd::global;
  using namespace hd::type;
  hd::util::ParseOptions(opt, argc, argv);
  if (opt.stride == 1) opt.fill_bit = 0;
  fillBit = std::to_string(opt.fill_bit).append(opt.separator);
  static LiveParser _live_parser;
  static int ctrlc = 0, max_ = 5;
  auto handler = [](int const signal) -> void {
    std::printf("%s\n", RED("\n正在退出..."));
    _live_parser.stopCapture();
    quit_guard(max_, ctrlc);
  };
  std::signal(SIGSTOP, handler);
  std::signal(SIGINT, handler);
  std::signal(SIGTERM, handler);
  std::signal(SIGKILL, handler);

  _live_parser.startCapture();
  return 0;
}
