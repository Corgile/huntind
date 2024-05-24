#include <csignal>
#include <unistd.h>
#include <hound/common/util.hpp>
#include <hound/common/global.hpp>
#include <hound/live_parser.hpp>
#include <hound/sink/kafka/producer_pool.hpp>

namespace hd::global {
type::capture_option opt;
ProducerPool producer_pool;
std::atomic_size_t NumBlockedFlows{0};
#if defined(BENCHMARK)
std::atomic<int32_t> packet_index = 0;
std::atomic<int32_t> num_captured_packet = 0;
std::atomic<int32_t> num_dropped_packets = 0;
std::atomic<int32_t> num_consumed_packet = 0;
std::atomic<int32_t> num_written_csv = 0;
#endif
}

using namespace hd::global;
using namespace hd::type;
using namespace hd::util::literals;
using namespace easylog;

static int pid{0};
static int ppid{0};
static int _signal{0};

static LiveParser* _live_parser{nullptr};

static void quit_guard(const int max_, int& ctrlc) {
  if (ctrlc++ <= 1) return;
  ELOG_INFO << RED("PID: [") << pid << RED("],PPID: [") << ppid << RED("] 后续事务进行中, 耐心等待！");
  if (ctrlc >= max_) {
    ELOG_INFO << YELLOW("程序结束状态: ") << hd::util::signal_msgs[_signal];
    exit(EXIT_SUCCESS);
  }
}

static void register_handler() {
  static int ctrlc = 0, patience = 10;
  auto handler = [](int const sig) -> void {
    std::printf("\x1b[2D");
    set_console(true);
    if (ctrlc == 0) {
      ELOG_INFO << RED("PID: [") << pid << RED("]  PPID: [") << ppid << RED("] 正在退出...");
    }
    if (_live_parser->isRunning()) {
      _live_parser->stopCapture();
    }
    quit_guard(patience, ctrlc);
    _signal = sig;
  };
  std::signal(SIGSTOP, handler);
  std::signal(SIGINT, handler);
  std::signal(SIGTERM, handler);
  std::signal(SIGKILL, handler);
}

static void init_parameters(const int argc, char* argv[]) {
  set_console(true);
  hd::util::ParseOptions(opt, argc, argv);
  pid = getpid();
  ppid = getppid();
  ELOG_INFO << CYAN("程序正在运行中: ") << RED("PID: ") << pid << RED(" PPID： ") << ppid;
  ELOG_INFO << "日志： tail -f ./log";
  opt.num_gpus = torch::cuda::device_count();
  producer_pool = ProducerPool(opt.poolSize, opt.brokers);
}

inline void main_loop() {
  set_console(false);
  _live_parser = new LiveParser();
  _live_parser->startCapture();
  delete _live_parser;
}

int main(const int argc, char* argv[]) {
  init_log(Severity::INFO, "log", true, false, 10_MB, 4, true);
  init_parameters(argc, argv);
  register_handler();
  main_loop();
  return 0;
}
