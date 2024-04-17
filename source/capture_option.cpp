//
// Created by brian on 11/22/23.
//

#include <iostream>
#include <hound/common/macro.hpp>
#include <hound/type/capture_option.hpp>

hd::type::capture_option::~capture_option() {
  if (verbose) {
    std::cout << "\t\t\t\t" << CYAN("运行完成， 没有bug我很开心") << "\n";
  }
}
