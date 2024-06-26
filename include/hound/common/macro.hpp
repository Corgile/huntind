//
// Created by brian on 11/22/23.
//

#ifndef HOUND_MACROS_HPP
#define HOUND_MACROS_HPP

#if defined(HD_ENABLE_LOG)

  #include <hound/dbg/dbg.hpp>
  #include <ylt/easylog.hpp>

#endif
#define Seconds(x)  std::chrono::seconds(x)
#define EmptyStatement (static_cast<void>(0))
#ifdef HD_LOG_LEVEL_TRACE
  #define ETRACE(x) x
#else
  #define ETRACE(x) EmptyStatement
#endif

#ifdef HD_LOG_LEVEL_DEBUG
  #define EDEBUG(x) x
#else
  #define EDEBUG(x) EmptyStatement
#endif

#ifdef HD_LOG_LEVEL_INFO
  #define EINFO(x) x
#else
  #define EINFO(x) EmptyStatement
#endif

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
#if defined(HD_LOG_LEVEL_TRACE)
  #define minSeverity Severity::TRACE
#elif defined(HD_LOG_LEVEL_DEBUG)
  #define minSeverity Severity::DEBUG
#elif defined(HD_LOG_LEVEL_INFO)
  #define minSeverity Severity::INFO
#elif defined(HD_LOG_LEVEL_WARN)
  #define minSeverity Severity::WARN
#elif defined(HD_LOG_LEVEL_ERROR)
  #define minSeverity Severity::ERROR
#elif defined(HD_LOG_LEVEL_FATAL)
  #define minSeverity Severity::FATAL
#endif
#ifdef HD_ENABLE_CONSOLE_LOG
  #define enableConsole true
  #define realTimeFlush false
#else
  #define enableConsole false
  #define realTimeFlush true
#endif
#pragma endregion 常量宏

#endif //HOUND_MACROS_HPP
