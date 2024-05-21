#include <csignal>
#include <unistd.h>
#include <hound/common/util.hpp>
#include <hound/common/global.hpp>
#include <hound/live_parser.hpp>
#include <hound/sink/kafka/producer_pool.hpp>

namespace hd::global {
type::capture_option opt;
ProducerPool producer_pool;
#if defined(BENCHMARK)
std::atomic<int32_t> packet_index = 0;
std::atomic<int32_t> num_captured_packet = 0;
std::atomic<int32_t> num_dropped_packets = 0;
std::atomic<int32_t> num_consumed_packet = 0;
std::atomic<int32_t> num_written_csv = 0;
#endif
}

static int pid{0};
static int ppid{0};

static void quit_guard(const int max_, int& ctrlc) {
  if (ctrlc++ <= 1) return;
  ELOG_INFO << RED("PID: [") << pid << RED("],PPID: [") << ppid << RED("] 后续事务进行中, 耐心等待！");
  if (ctrlc >= max_) exit(EXIT_SUCCESS);
}

int main(const int argc, char* argv[]) {
  using namespace hd::global;
  using namespace hd::type;
  using namespace hd::util::literals;
  using namespace easylog;

  hd::util::ParseOptions(opt, argc, argv);
  opt.num_gpus = torch::cuda::device_count();
  producer_pool = ProducerPool(opt.poolSize, opt.brokers);

  static int ctrlc = 0, patience = 10;
  static int _signal{0};
  pid = getpid();
  ppid = getppid();

  /// _live_parser的初始化一定要比easylog::logger：：init_log（）后
  static LiveParser* _live_parser{nullptr};
  auto handler = [](int const sig) -> void {
    set_console(true);
    std::printf("\x1b[2D");
    if (ctrlc == 0) {
      ELOG_INFO << RED("PID: [") << pid << RED("],PPID: [") << ppid << RED("] 正在退出...");
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

  ELOG_INFO << CYAN("程序正在运行中");
  ELOG_INFO << RED("PID: ") << pid << RED(", PPID： ") << ppid;
  ELOG_INFO << "日志： tail -f ./log";

  /// _live_parser的初始化一定要比easylog::logger：：init_log（）后
  init_log(Severity::INFO, "log", true, false, 10_MB, 4);
  _live_parser = new LiveParser();
  _live_parser->startCapture();
  set_console(true);
  delete _live_parser;
  ELOG_INFO << YELLOW("程序结束状态: ") << hd::util::signal_msgs[_signal];
  ELOG_INFO << RED("发送本地Kafka队列中剩余的消息, 请等待....");
  return 0;
}
