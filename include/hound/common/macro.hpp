//
// Created by brian on 11/22/23.
//

#ifndef HOUND_MACROS_HPP
#define HOUND_MACROS_HPP

#if defined(HD_DEV)
#include <dbg.h>
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
#define RED(x)     "\033[31;1m" x "\033[0m"
#define GREEN(x)   "\033[32;1m" x "\033[0m"
#define YELLOW(x)  "\033[33;1m" x "\033[0m"
#define BLUE(x)    "\033[34;1m" x "\033[0m"
#define CYAN(x)    "\033[36;1m" x "\033[0m"
#endif //HD_ANSI_COLOR
#pragma endregion 常量宏

#pragma region 功能性宏

/// 仅在开发阶段作为调试使用
#ifndef hd_debug
#if defined(HD_DEV)
#define hd_debug(...)  dbg(__VA_ARGS__)
#else//- not HD_DEV
#define hd_debug(...)
#endif
#endif//- hd_debug

#pragma endregion 功能性宏
#endif //HOUND_MACROS_HPP
