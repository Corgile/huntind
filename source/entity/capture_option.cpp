//
// Created by brian on 11/22/23.
//

#include <hound/common/macro.hpp>
#include <hound/type/capture_option.hpp>

// @formatter:off
void hd::type::capture_option::print() const {
  std::printf("\n%s\n", CYAN("包含字段: "));
  if (payload > 0)      std::printf(CYAN("payload")"（%d)", payload);
  if (include_pktlen)   std::printf(",%s", CYAN("报文长度"));
  if (include_ts)       std::printf(",%s", CYAN("时间戳"));
  std::printf("%s\n", ";");
  if (num_packets > 0)  std::printf("%s%d\n", CYAN("抓包个数: "), num_packets);
  std::printf("%s%d%s\n", CYAN("填充值: "), fill_bit, CYAN("; "));
  std::printf("%s%d%s",CYAN("将每 "), stride, CYAN(" 位一组按"));
  std::printf("%s%s\n", YELLOW("无符号"), CYAN("类型转换为10进制;"));
  std::printf("%s%d\n", CYAN("包处理线程: "), workers);
  std::printf("%s%s\n",CYAN("包过滤表达式: "), filter.c_str());
  //@formatter:on
  if (this->write_file and not output_file.empty()) {
    std::printf("%s%s", CYAN("输出文件:  "), output_file.c_str());
  }
}

hd::type::capture_option::~capture_option() {
  if (verbose) {
    print();
  }
}
