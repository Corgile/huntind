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

public:
  explicit TextFileSink(const std::string& fileName) :
    mDataFile(fileName + ".data",   std::ios::out | std::ios::app),
    mLabelFile(fileName + ".label", std::ios::out | std::ios::app) {
    hd_debug(__PRETTY_FUNCTION__);
    const auto parent = absolute(fs::path(fileName)).parent_path();
    if (not exists(parent)) {
      create_directories(parent);
    }
    bool const isGoodData = mDataFile.good();
    bool const isGoodLabel = mLabelFile.good();
    if (not isGoodData or not isGoodLabel) {
      hd_line(RED("无法打开输出文件: "), fileName);
      exit(EXIT_FAILURE);
    }
  }

  /// 写入文本文件(csv)
  void consumeData(ParsedData const& data) override {
    if (not data.HasContent) return;
    if (not global::_5tupleAttackTypeDic.contains(data.m5Tuple)) return;
    std::string buffer;
    this->fillCsvBuffer(data, buffer);
    {
      std::scoped_lock file(fileWriteAccess);
      const auto benign = global::_5tupleAttackTypeDic.at(data.m5Tuple).compare("BENIGN");
      if (benign == 0) this->mLabelFile << "0";
      else this->mLabelFile << "1";
      this->mDataFile << buffer << "\n";
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
