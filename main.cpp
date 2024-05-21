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
  set_console(true);
  ELOG_INFO << RED("PID: [") << pid << RED("],PPID: [") << ppid << RED("] 后续事务进行中, 耐心等待！");
  set_console(false);
  if (ctrlc >= max_) exit(EXIT_SUCCESS);
}

static void register_handler() {
  static int ctrlc = 0, patience = 10;
  auto handler = [](int const sig) -> void {
    std::printf("\x1b[2D");
    if (ctrlc == 0) {
      set_console(true);
      ELOG_INFO << RED("PID: [") << pid << RED("],PPID: [") << ppid << RED("] 正在退出...");
      set_console(false);
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
  init_log(Severity::INFO, "log", true, false, 10_MB, 4);
  hd::util::ParseOptions(opt, argc, argv);
  opt.num_gpus = torch::cuda::device_count();
  producer_pool = ProducerPool(opt.poolSize, opt.brokers);
  pid = getpid();
  ppid = getppid();
}

static void verbose_message() {
  set_console(true);
  ELOG_INFO << CYAN("程序正在运行中: ") << RED("PID: ") << pid << RED(", PPID： ") << ppid;
  ELOG_INFO << "日志： tail -f ./log";
  set_console(false);
}

static void main_loop() {
  set_console(false);
  set_console(false);
  set_console(false);
  set_console(false);
  _live_parser = new LiveParser();
  _live_parser->startCapture();
}

int main(const int argc, char* argv[]) {

  init_parameters(argc, argv);
  register_handler();
  verbose_message();
  main_loop();

  set_console(false);
  delete _live_parser;

  set_console(true);
  ELOG_INFO << YELLOW("程序结束状态: ") << hd::util::signal_msgs[_signal];
  ELOG_INFO << RED("发送本地Kafka队列中剩余的消息, 请等待....");
  set_console(false);

  return 0;
}
