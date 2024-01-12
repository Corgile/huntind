#include <csignal>
#include <hound/common/util.hpp>
#include <hound/common/global.hpp>

#include <hound/parser/dead_parser.hpp>
#include <hound/csv-helper.hpp>

namespace hd::global {
type::capture_option opt;
std::string fillBit;
std::unordered_map<std::string, std::string> id_type = {};
}

void static InitDict() {
  io::CSVReader<6> in(hd::global::opt.csv);
  in.read_header(io::ignore_extra_column,
                 "Source IP", "Source Port",
                 "Destination IP", "Destination Port",
                 "Protocol", "Label");

  std::string SourceIP;
  std::string SourcePort;
  std::string DestinationIP;
  std::string DestinationPort;
  std::string Protocol;
  std::string Label;
  while (in.read_row(SourceIP, SourcePort,
                     DestinationIP, DestinationPort,
                     Protocol, Label)) {
    // do stuff with the data
    std::stringstream key;
    key
      << SourceIP << "-" << DestinationIP << "-"
      << SourcePort << "-"
      << DestinationPort << "-" << Protocol;
    // 相信数据集对于同样的flowID, 其label不会不同， 所以直接覆盖
    hd::global::id_type[std::move(key.str())].assign(Label);
  }
}

int main(const int argc, char* argv[]) {
  using namespace hd::global;
  using namespace hd::type;
  hd::util::ParseOptions(opt, argc, argv);
  InitDict();
  static std::unique_ptr<DeadParser> deadParser{nullptr};
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
  deadParser = std::make_unique<DeadParser>();
  deadParser->processFile();
  return 0;
}
