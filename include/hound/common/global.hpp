//
// Created by brian on 11/22/23.
//

#ifndef HOUND_GLOBAL_HPP
#define HOUND_GLOBAL_HPP

#include <unordered_map>
#include <hound/type/capture_option.hpp>

namespace hd::global {
extern type::capture_option opt;
extern std::string fillBit;
extern std::unordered_map<std::string,std::string> _5tupleAttackTypeDic;
}

#endif //HOUND_GLOBAL_HPP
