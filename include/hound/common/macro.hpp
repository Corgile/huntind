//
// Created by brian on 11/22/23.
//

#ifndef HOUND_MACROS_HPP
#define HOUND_MACROS_HPP

#include <dbg.h>
#include <iostream>

namespace hd::macro {
static std::mutex coutMutex;

template <typename ...T>
static void printL(T ...args) {
  ((std::cout << args), ...);
}

#pragma region 常量宏
#ifndef XXX_PADSIZE
#define IP4_PADSIZE  60
#define TCP_PADSIZE  60
#define UDP_PADSIZE  8
#define XXX_PADSIZE
#endif// XXX_PADSIZE
#pragma endregion 常量宏

#pragma region 功能性宏


#ifndef HD_ANSI_COLOR
#define HD_ANSI_COLOR
#define RED(x)     "\033[31;1m" x "\033[0m"
#define GREEN(x)   "\033[32;1m" x "\033[0m"
#define YELLOW(x)  "\033[33;1m" x "\033[0m"
#define BLUE(x)    "\033[34;1m" x "\033[0m"
#define CYAN(x)    "\033[36;1m" x "\033[0m"
#endif //HD_ANSI_COLOR

#ifndef hd_info
#define hd_info(...)       			        \
do {                     				        \
std::lock_guard lock(macro::coutMutex); \
hd::macro::printL(__VA_ARGS__);         \
} while (false)
#endif//-hd_info

#ifndef hd_line
#define hd_line(...)       			            \
do {                     				            \
std::lock_guard lock(hd::macro::coutMutex); \
hd::macro::printL(__VA_ARGS__, "\n");       \
} while (false)
#endif//-hd_line

/// 仅在开发阶段作为调试使用
#ifndef hd_debug
#if defined(HD_DEV)
#define hd_debug(...)  dbg(__VA_ARGS__)
#else//- not HD_DEV
#define hd_debug(...)
#endif
#endif//- hd_debug

#pragma endregion 功能性宏
} // namespace hd::macro
#endif //HOUND_MACROS_HPP
