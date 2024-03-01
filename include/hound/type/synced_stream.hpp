//
// Created by brian on 11/29/23.
//

#ifndef HOUND_SYNCED_STREAM_HPP
#define HOUND_SYNCED_STREAM_HPP

#include <mutex>

namespace hd::type {
template <typename StreamType>
class SyncedStream {
public:
  SyncedStream(const std::string& filename, std::ios_base::openmode mode)
    : mOutStream(filename, mode) {}

  template <typename ...Args>
  SyncedStream(const std::string& filename, std::ios_base::openmode mode, Args&& ...args) {
    mOutStream = StreamType(filename, mode, std::forward<Args>(args) ...);
  }

  SyncedStream(StreamType&& stream)
    : mOutStream(std::forward<StreamType>(stream)) {}

  /// 同步访问流对象的成员函数
  template <typename Func, typename ...Args>
  auto SyncInvoke(Func&& func, Args&& ...args) {
    std::lock_guard lock(mutex_);
    // return std::invoke(std::forward<Func>(func), mOutStream, std::forward<Args>(args)...);
    return std::forward<Func>(func)(mOutStream, std::forward<Args>(args) ...);
  }

  template <typename Func, typename ...Args>
  auto invoke(Func&& func, Args&& ...args) {
    // return std::invoke(std::forward<Func>(func), mOutStream, std::forward<Args>(args)...);
    return std::forward<Func>(func)(mOutStream, std::forward<Args>(args) ...);
  }

  template <typename T>
  // SyncedStream& operator<<(const T& data) {
  void operator<<(const T& data) {
    std::scoped_lock lock(mutex_);
    mOutStream << data << '\n';
    // 不返回*this是因为外部调用 << 时:
    // filestream << content << other;
    // 两个 << 调用彼此之间并不是sync的, 会发生竞争 导致unexpected b
    // return *this;
  }

  /// 特化版本处理操纵符，处理 std::endl, std::flush等
  SyncedStream& operator<<(std::ostream& (*manipulator)(std::ostream&)) {
    std::scoped_lock<std::mutex> lock(mutex_);
    manipulator(mOutStream);
    return *this;
  }

private:
  StreamType mOutStream;
  mutable std::mutex mutex_;
};
} // type
// hd

#endif //HOUND_SYNCED_STREAM_HPP
