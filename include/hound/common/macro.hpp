//
// Created by brian on 11/22/23.
//

#ifndef HOUND_MACROS_HPP
#define HOUND_MACROS_HPP

#if defined(HD_LOG_ERROR) || defined(HD_LOG_WARN) || defined(HD_LOG_INFO) || defined(HD_LOG_DEBUG)
  #include <hound/dbg/dbg.hpp>
  #include <ylt/easylog.hpp>
#endif
#define Seconds(x)  std::chrono::seconds(x)

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

#endif //HOUND_MACROS_HPP
