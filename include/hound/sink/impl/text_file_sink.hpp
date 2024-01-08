//
// Created by brian on 11/22/23.
//

#ifndef HOUND_TEXT_FILE_SINK_HPP
#define HOUND_TEXT_FILE_SINK_HPP

#include <string>
#include <fstream>
#include <filesystem>

#include <hound/sink/base_sink.hpp>
#include <hound/type/synced_stream.hpp>

namespace hd ::type {
namespace fs = std::filesystem;

class TextFileSink : public BaseSink {
private:
  SyncedStream<std::ofstream> mOutFile;

public:
  explicit TextFileSink(const std::string& fileName) :
    mOutFile(fileName, std::ios::out) {
    hd_debug(__PRETTY_FUNCTION__);
    auto parent = absolute(fs::path(fileName)).parent_path();
    if (not exists(parent)) {
      create_directories(parent);
    }
    bool const isGood = mOutFile.invoke([](std::ofstream const& stream) {
      return stream.good();
    });

    if (not isGood) {
      hd_line(RED("无法打开输出文件: "), fileName);
      exit(EXIT_FAILURE);
    }
  }

  /// 写入文本文件(csv)
  void consumeData(ParsedData const& data) override {
    if (not data.HasContent) return;
    std::string buffer;
    this->fillCsvBuffer(data, buffer);
    this->mOutFile << std::move(buffer);
#if defined(BENCHMARK)
    ++global::num_written_csv;
#endif
  }
};
} // entity

#endif //HOUND_TEXT_FILE_SINK_HPP
