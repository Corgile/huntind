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

int main(const int argc, char* argv[]) {
  using namespace hd::global;
  using namespace hd::type;
  hd::util::ParseOptions(opt, argc, argv);
  if (opt.stride == 1) opt.fill_bit = 0;
  fillBit = std::to_string(opt.fill_bit).append(opt.separator);

  static int ctrlc = 0, max_ = 5;
  auto handler = [](int const signal) -> void {
    if (signal == SIGINT) {
      auto const more = max_ - ++ctrlc;
      if (more > 0) {
        hd_line(RED("\n再按 "), more, RED(" 次 [Ctrl-C] 退出"));
      }
      if (ctrlc >= max_) {
        exit(EXIT_FAILURE);
      }
    }
    if (signal == SIGTERM) {
      hd_line(RED("\n[SIGTERM] received. 即将退出..."));
    }
    if (signal == SIGKILL) {
      hd_line(RED("\n[SIGKILL] received. 即将退出..."));
    }
  };
  std::signal(SIGSTOP, handler);
  std::signal(SIGINT, handler);
  std::signal(SIGTERM, handler);
  std::signal(SIGKILL, handler);
  LiveParser _live_parser;
  _live_parser.startCapture();
  return 0;
}
