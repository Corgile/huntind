//
// Created by brian on 11/22/23.
//

#ifndef HOUND_TEXT_FILE_SINK_HPP
#define HOUND_TEXT_FILE_SINK_HPP

#include <string>
#include <fstream>
#include <filesystem>

#include <hound/sink/base_sink.hpp>

namespace hd ::type {
namespace fs = std::filesystem;

class TextFileSink final : public BaseSink {
private:
  std::ofstream mDataFile;
  std::ofstream mLabelFile;
  std::mutex fileWriteAccess;
  bool _firstone{false};
  uint32_t sec{}, uSec{};

public:
  explicit TextFileSink(const std::string& fileName) :
    mDataFile(fileName + ".data",   std::ios::out | std::ios::app),
    mLabelFile(fileName + ".label", std::ios::out | std::ios::app) {
    hd_debug(__PRETTY_FUNCTION__);
    const auto parent = absolute(fs::path(fileName)).parent_path();
    if (not exists(parent)) {
      create_directories(parent);
    }
    bool const isGoodData  = mDataFile.good();
    bool const isGoodLabel = mLabelFile.good();
    if (not isGoodData or not isGoodLabel) {
      hd_line(RED("无法打开输出文件: "), fileName);
      exit(EXIT_FAILURE);
    }
  }

  /// 写入文本文件(csv)
  void consumeData(ParsedData const& data) override {
    using global::id_type;
    if (not data.HasContent) return;
    if (not id_type.contains(data.m5Tuple)) return;
    if (not _firstone) {
      sec = data.Sec;
      uSec = data.uSec;
      _firstone = not _firstone;
    }
    const uint32_t pkt_rtime{(data.Sec - sec) * 1000 + (data.uSec - uSec) / 1000};
    {
      std::scoped_lock file(fileWriteAccess);
      this->mLabelFile << (id_type.at(data.m5Tuple) not_eq "BENIGN");
      this->mDataFile
        << data.version << ' '
        << data.sIP << ' '
        << data.dIP << ' '
        << data.sPort << ' '
        << data.dPort << ' '
        << pkt_rtime << ' '
        << data.pktCode << ' '
        << data.capLen << '\n';
    }
  }

  ~TextFileSink() override {
    mDataFile.flush();
    mDataFile.close();
    mLabelFile.flush();
    mLabelFile.close();
  };
};
} // entity

#endif //HOUND_TEXT_FILE_SINK_HPP
