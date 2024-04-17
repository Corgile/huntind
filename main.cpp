#include <csignal>
#include <hound/common/util.hpp>
#include <hound/common/global.hpp>

#include <hound/live_parser.hpp>
#include <hound/sink/kafka/producer_pool.hpp>

#include <c10/util/Exception.h>
#include <unistd.h> // 包含getpid()

namespace hd::global {
type::capture_option opt;
ProducerPool producer_pool;
// type::ModelPool model_pool;
torch::jit::Module* pModel_;
torch::jit::Module model_;
// torch::Device calc_device(torch::kCUDA, 5);
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
  using namespace hd::util::literals;

  easylog::set_async(true);
  hd::util::ParseOptions(opt, argc, argv);
  // calc_device = torch::Device(torch::kCUDA, opt.cudaId);
  producer_pool = ProducerPool(opt.poolSize, opt.brokers);
  // model_pool = ModelPool(opt.model_path);
  opt.num_gpus = torch::cuda::device_count();
  model_ = torch::jit::load(opt.model_path);
  if (opt.stride == 1) opt.fill_bit = 0;

  static LiveParser _live_parser;
  static int ctrlc = 0, max_ = 5;
  static int _signal{0};
  static int pid = getpid();
  static int ppid = getppid();
  auto handler = [](int const sig) -> void {
    easylog::set_console(true);
    std::printf("\x1b[2D");
    ELOG_INFO << RED("PID: [") << pid << RED("],PPID: [") << ppid << RED("] 正在退出...");
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

  ELOG_INFO << "进程：" RED("PID[") << pid << RED("] PPID[") << ppid << CYAN("已开始运行。 你可以通过 tail -f ./log 查看日志。");
  easylog::init_log(easylog::Severity::INFO, "log", true, false, 10_MB, 4);
  try {
    _live_parser.startCapture();
  } catch (const c10::Error&) {
    ELOG_ERROR << RED("发生一个torch内部错误");
  }
  easylog::set_console(true);
  ELOG_INFO << YELLOW("程序退出信号: ") << hd::util::signal_msgs[_signal];
  return 0;
}
