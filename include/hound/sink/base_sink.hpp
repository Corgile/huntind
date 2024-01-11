//
// Created by brian on 11/28/23.
//

#ifndef HOUND_BASE_SINK_HPP
#define HOUND_BASE_SINK_HPP

#include <hound/type/parsed_data.hpp>
#include <hound/type/synced_stream.hpp>

namespace hd::type {
/**
 * 默认流量处理：打印控制台
 */
class BaseSink {
  SyncedStream<std::ostream&> mConsole;

  bool _firstone{false};
  uint32_t sec, uSec;

public:
  BaseSink(std::string const&) : mConsole(std::cout) {
  }

  BaseSink() : mConsole(std::cout) {
  }

  virtual void consumeData(ParsedData const& data) {
    // TODO: 异步
    if (not data.HasContent) return;
    std::string buffer;
#if defined(HD_DEV)
    hd_line(std::move(buffer));
#else
    mConsole << buffer;
#endif
  }

  virtual ~BaseSink() = default;

protected:
  void fillCsvBuffer(ParsedData const& data, std::string& buffer) {
    if (not _firstone) {
      sec = data.Sec;
      uSec = data.uSec;
    }
    const uint32_t pkt_rtime = (data.Sec - sec) * 1000 + (data.uSec - uSec) / 1000;
    buffer.append(std::to_string(data.version)).append(" ")
          .append(std::to_string(data.sIP)).append(" ")
          .append(std::to_string(data.dIP)).append(" ")
          .append(std::to_string(data.sPort)).append(" ")
          .append(std::to_string(data.dPort)).append(" ")
          .append(std::to_string(pkt_rtime)).append(" ")
          .append(std::to_string(data.pktCode)).append(" ")
          .append(std::to_string(data.capLen));
    _firstone = true;
  }
};
} // entity

#endif //HOUND_BASE_SINK_HPP
