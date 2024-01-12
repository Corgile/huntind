#include <csignal>
#include <hound/common/util.hpp>
#include <hound/common/global.hpp>

#include <hound/parser/dead_parser.hpp>

namespace hd::global {
type::capture_option opt;
std::string fillBit;
std::atomic<int32_t> packet_index = 0;
std::atomic<int32_t> num_captured_packet = 0;
std::atomic<int32_t> num_dropped_packets = 0;
std::atomic<int32_t> num_consumed_packet = 0;
std::atomic<int32_t> num_written_csv = 0;
}

int main(const int argc, char* argv[]) {
  using namespace hd::global;
  using namespace hd::type;
  hd::util::ParseOptions(opt, argc, argv);
  if (opt.unsign or opt.stride == 1) opt.fill_bit = 0;
  fillBit = std::to_string(opt.fill_bit).append(opt.separator);
  DeadParser deadParser;
  deadParser.processFile();
  return 0;
}
