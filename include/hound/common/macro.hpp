//
// Created by brian on 11/22/23.
//

#ifndef HOUND_MACROS_HPP
#define HOUND_MACROS_HPP

#if defined(HD_LOG_ERROR) || defined(HD_LOG_WARN) || defined(HD_LOG_INFO) || defined(HD_LOG_DEBUG)
  #include <dbg/dbg.hpp>
  #include <ylt/easylog.hpp>
#endif
#include <iostream>
#include <mutex>

#pragma region 常量
#ifndef XXX_PADSIZE
  #define IP4_PADSIZE  60
  #define TCP_PADSIZE  60
  #define UDP_PADSIZE  8
  #define XXX_PADSIZE
#endif// XXX_PADSIZE
#ifndef HD_ANSI_COLOR
  #define HD_ANSI_COLOR
  #define RED(x)     "\x1b[31;1m" x "\033[0m"
  #define GREEN(x)   "\x1b[32;1m" x "\033[0m"
  #define YELLOW(x)  "\x1b[33;1m" x "\033[0m"
  #define BLUE(x)    "\x1b[34;1m" x "\033[0m"
  #define CYAN(x)    "\x1b[36;1m" x "\033[0m"
  #define WHITE(x)   "\x1b[38;1m" x "\033[0m"
#endif //HD_ANSI_COLOR
#pragma endregion 常量宏

#pragma region 功能性宏

namespace hd::macro {
inline std::mutex coutMutex;

template<typename ...T>
inline void printL(T ...args) {
  ((std::cout << args), ...);
}
}

#ifndef hd_println
  #define hd_println(...)                     \
  do {                                        \
  std::lock_guard lock(hd::macro::coutMutex); \
  hd::macro::printL(__VA_ARGS__, "\n");       \
  } while (false)
#endif//-hd_println
#ifndef hd_print
  #define hd_print(...)                       \
  do {                                        \
  std::lock_guard lock(hd::macro::coutMutex); \
  hd::macro::printL(__VA_ARGS__);       \
  } while (false)
#endif//-hd_print

#if defined(HD_LOG_ERROR)
  #define hd_error(x, ...)  dbg(RED(x),     __VA_ARGS__)
  #define hd_warn(x, ...)
  #define hd_info(x, ...)
  #define hd_debug(x, ...)
#elif defined(HD_LOG_WARN)
  #define hd_error(x, ...)  dbg(RED(x),     __VA_ARGS__)
  #define hd_warn(x, ...)   dbg(YELLOW(x),  __VA_ARGS__)
  #define hd_info(x, ...)
  #define hd_debug(x, ...)
#elif defined(HD_LOG_INFO)
  #define hd_error(x, ...)  dbg(RED(x),     __VA_ARGS__)
  #define hd_warn(x, ...)   dbg(YELLOW(x),  __VA_ARGS__)
  #define hd_info(x, ...)   dbg(CYAN(x),    __VA_ARGS__)
  #define hd_debug(x, ...)
#elif defined(HD_LOG_DEBUG)
  #define hd_error(x, ...)  dbg(RED(x),     __VA_ARGS__)
  #define hd_warn(x, ...)   dbg(YELLOW(x),  __VA_ARGS__)
  #define hd_info(x, ...)   dbg(CYAN(x),    __VA_ARGS__)
  #define hd_debug(x, ...)  dbg(WHITE(x),   __VA_ARGS__)
  #undef hd_debug
#endif

#pragma endregion 功能性宏
#endif //HOUND_MACROS_HPP
