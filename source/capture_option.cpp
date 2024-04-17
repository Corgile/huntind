//
// Created by brian on 11/22/23.
//

#include "hound/common/macro.hpp"
#include "hound/type/capture_option.hpp"

hd::type::capture_option::~capture_option() {
  if (verbose) {
    easylog::set_console(true);
    ELOG_INFO << CYAN("运行完成");
  }
}
