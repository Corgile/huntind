//
// Created by brian on 11/28/23.
//

#ifndef HOUND_BASE_SINK_HPP
#define HOUND_BASE_SINK_HPP

#include <hound/type/parsed_data.hpp>
#include <hound/common/core.hpp>
#include <hound/common/global.hpp>
#include <hound/type/synced_stream.hpp>

namespace hd::type {
/**
 * 默认流量处理：打印控制台
 */
class BaseSink {
  SyncedStream<std::ostream&> mConsole;

public:
  BaseSink(std::string const&) : mConsole(std::cout) {}

  BaseSink() : mConsole(std::cout) {}

  virtual void consumeData(ParsedData const& data) {
    // TODO: 异步
    if (not data.HasContent) return;
    std::string buffer;
    hd::core::util::fillCsvBuffer(data, buffer);
#if defined(HD_DEV)
    hd_println(std::move(buffer));
#else
    mConsole << buffer;
#endif
  }

  virtual ~BaseSink() = default;
};
} // entity

#endif //HOUND_BASE_SINK_HPP
