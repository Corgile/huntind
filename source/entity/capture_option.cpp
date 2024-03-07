//
// Created by brian on 11/22/23.
//

#include <hound/common/macro.hpp>
#include <hound/type/capture_option.hpp>

// @formatter:off
void hd::type::capture_option::print() const {
  std::stringstream ss;
  ss << "\n" << CYAN("包含字段: ") << "\n";
  if (payload > 0)      ss << CYAN("payload") << "（" << payload << ")";
  if (include_pktlen)   ss << "," << CYAN("报文长度");
  if (include_ts)       ss << "," << CYAN("时间戳");
  ss << CYAN(";") << "\n";
  if (num_packets > 0)  ss << CYAN("抓包个数: ") << num_packets << "\n";
  ss << CYAN("填充值: ") << fill_bit << CYAN("; ") << "\n";
  ss << CYAN("将每 ") << stride << CYAN(" 位一组按");
  ss << YELLOW("无符号") << CYAN("类型转换为10进制;") << "\n";
  ss << CYAN("包处理线程: ") << workers << "\n";
  ss << CYAN("包过滤表达式: ") << filter << "\n";
  if (write_file && !output_file.empty()) {
    ss << CYAN("输出文件:  ") << output_file;
  }
  ELOG_INFO << ss;
}

hd::type::capture_option::~capture_option() {
  if (verbose) {
    print();
  }
}
