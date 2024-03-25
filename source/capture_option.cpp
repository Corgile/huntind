//
// Created by brian on 11/22/23.
//

#include "../include/hound/common/macro.hpp"
#include "../include/hound/type/capture_option.hpp"

// @formatter:off
void hd::type::capture_option::print() const {
  std::cout << "\n" << CYAN("包含字段: ") << "\n";
  if (payload > 0)      std::cout << CYAN("payload") << "（" << payload << ")";
  if (include_pktlen)   std::cout << "," << CYAN("报文长度");
  if (include_ts)       std::cout << "," << CYAN("时间戳");
  std::cout << CYAN(";") << "\n";
  if (num_packets > 0)  std::cout << CYAN("抓包个数: ") << num_packets << "\n";
  std::cout << CYAN("填充值: ") << fill_bit << CYAN("; ") << "\n";
  std::cout << CYAN("将每 ") << stride << CYAN(" 位一组按");
  std::cout << YELLOW("无符号") << CYAN("类型转换为10进制;") << "\n";
  std::cout << CYAN("包处理线程: ") << workers << "\n";
  std::cout << CYAN("包过滤表达式: ") << filter << "\n";
  if (write_file && !output_file.empty()) {
    std::cout << CYAN("输出文件:  ") << output_file;
  }
}

hd::type::capture_option::~capture_option() {
  if (verbose) {
    print();
  }
}
