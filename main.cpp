#include <csignal>
#include <unistd.h>
#include <hound/common/util.hpp>
#include <hound/common/global.hpp>
#include <hound/live_parser.hpp>
#include <hound/sink/kafka/producer_pool.hpp>

namespace hd::global {
type::capture_option opt;
ProducerPool producer_pool;
std::atomic<int64_t> NumBlockedFlows{0};

int max_send_batch = 1500;
int max_encode_batch = 1500;
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
static int Signal{0};
bool enable_console{false};

static LiveParser* pLiveParser{nullptr};

static void quit_guard(const int max_, int& ctrlc) {
  if (ctrlc++ <= 1) return;
  ELOG_INFO << RED("PID: [") << pid << RED("],PPID: [") << ppid << RED("] 后续事务进行中, 耐心等待！");
  if (ctrlc >= max_) {
    ELOG_INFO << YELLOW("程序结束状态: ") << hd::util::signal_msgs[Signal];
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
    if (pLiveParser->isRunning()) {
      pLiveParser->stopCapture();
    }
    quit_guard(patience, ctrlc);
    Signal = sig;
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
#ifdef HD_LOG_LEVEL_INFO
  ELOG_INFO << CYAN("程序正在运行中: ") << RED("PID: ") << pid << RED(" PPID： ") << ppid;
  ELOG_INFO << "日志： tail -f ./log";
#endif
  /// 获取系统中可用的GPU数量
  opt.num_gpus = torch::cuda::device_count();
  /// 获取系统中可用的CPU数量
  opt.num_cpus = std::thread::hardware_concurrency();
  producer_pool = ProducerPool(opt.poolSize, opt.brokers);
}

inline void main_loop() {
  set_console(enable_console);
  pLiveParser = new LiveParser();
  pLiveParser->startCapture();
  delete pLiveParser;
}

int main(const int argc, char* argv[]) {
  easylog::Severity min_severity;
#if defined(HD_LOG_LEVEL_TRACE)
  min_severity = Severity::TRACE;
#elif defined(HD_LOG_LEVEL_DEBUG)
  min_severity = Severity::DEBUG;
#elif defined(HD_LOG_LEVEL_INFO)
  min_severity = Severity::INFO;
#elif defined(HD_LOG_LEVEL_WARN)
  min_severity = Severity::WARN;
#elif defined(HD_LOG_LEVEL_ERROR)
  min_severity = Severity::ERROR;
#elif defined(HD_LOG_LEVEL_FATAL)
  min_severity = Severity::FATAL;
#endif
#ifdef HD_ENABLE_CONSOLE_LOG
  enable_console = true;
#endif
  init_log(min_severity, "log", true, enable_console, 10_MB, 4, !enable_console);
  init_parameters(argc, argv);
  register_handler();
  main_loop();
  return 0;
}
