#include <csignal>
#include <hound/common/util.hpp>
#include <hound/common/global.hpp>

#include <hound/live_parser.hpp>
#include <hound/sink/kafka/producer_pool.hpp>

#include <c10/util/Exception.h>

namespace hd::global {
type::capture_option opt;
ProducerPool producer_pool;
type::ModelPool model_pool;
#if defined(BENCHMARK)
std::atomic<int32_t> packet_index = 0;
std::atomic<int32_t> num_captured_packet = 0;
std::atomic<int32_t> num_dropped_packets = 0;
std::atomic<int32_t> num_consumed_packet = 0;
std::atomic<int32_t> num_written_csv = 0;
#endif
}

static void quit_guard(const int max_, int& ctrlc) {
  if (++ctrlc >= max_) {
    exit(EXIT_FAILURE);
  }
}

int main(const int argc, char* argv[]) {
  using namespace hd::global;
  using namespace hd::type;
  if (argc <= 1) [[unlikely]]{
    hd::util::Doc();
    exit(EXIT_SUCCESS);
  }
  hd::util::ParseOptions(opt, argc, argv);
  if (opt.stride == 1) opt.fill_bit = 0;
  producer_pool = ProducerPool(opt.poolSize, opt.brokers);
  model_pool = ModelPool(20, opt.model_path);

  static LiveParser _live_parser;
  static int ctrlc = 0, max_ = 5;
  static int _signal{0};
  auto handler = [](int const sig) -> void {
    std::printf("\x1b[2D");
    ELOG_INFO << RED("正在退出...");
    if (_live_parser.is_running) {
      _live_parser.stopCapture();
    }
    quit_guard(max_, ctrlc);
    _signal = sig;
  };
  std::signal(SIGSTOP, handler);
  std::signal(SIGINT, handler);
  std::signal(SIGTERM, handler);
  std::signal(SIGKILL, handler);

  easylog::set_min_severity(easylog::Severity::DEBUG);
  easylog::set_async(true);
  ELOG_INFO << GREEN("已经开始捕获流消息....");
  try {
    _live_parser.startCapture();
  } catch (const c10::Error&) {
    ELOG_ERROR << RED("发生一个torch内部错误");
  }
  ELOG_INFO << YELLOW("程序退出信号: ") << _signal;
  return 0;
}
