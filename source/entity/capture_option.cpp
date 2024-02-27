//
// Created by brian on 11/22/23.
//

#include <hound/common/macro.hpp>
#include <hound/type/capture_option.hpp>

// @formatter:off
void hd::type::capture_option::print() const {
  hd_info(CYAN("\n包含字段: "));
  if (payload > 0)      hd_info(CYAN("payload"),"（", payload, ")");
  if (include_pktlen)   hd_info(CYAN(",报文长度"));
  if (include_ts)       hd_info(CYAN(",时间戳"));
  hd_line(";");
  if (num_packets > 0)  hd_line(CYAN("抓包个数: "), num_packets);
  hd_line(CYAN("填充值: "), fill_bit, CYAN("; "));
  hd_info(CYAN("将每 "), stride, CYAN(" 位一组按"));
  hd_line(YELLOW("无符号") CYAN("类型转换为10进制;"));
  hd_line(CYAN("包处理线程: "), workers);
  hd_line(CYAN("包过滤表达式: "), filter);
  //@formatter:on
  if (this->write_file and not output_file.empty()) {
    hd_line(CYAN("输出文件:  "), output_file);
  }
}

hd::type::capture_option::~capture_option() {
  if (verbose) {
    print();
  }
}
