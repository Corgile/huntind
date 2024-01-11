//
// Created by brian on 11/22/23.
//
#include <hound/common/util.hpp>
#include <hound/common/macro.hpp>
#include <hound/parser/dead_parser.hpp>
#include <hound/sink/impl/text_file_sink.hpp>


hd::type::DeadParser::DeadParser() {
  this->mHandle = util::OpenDeadHandle(global::opt, this->mLinkType);
  mSink.reset(new TextFileSink(global::opt.out));
}

void hd::type::DeadParser::processFile() {
  using namespace hd::global;
  for (int i = 0; i < opt.worker; ++i) {
    std::thread(&DeadParser::consumer_job, this).detach();
  }
  pcap_loop(mHandle, opt.num, deadHandler, reinterpret_cast<byte_t*>(this));
}

void hd::type::DeadParser::deadHandler(byte_t* user_data, const pcap_pkthdr* pkthdr, const byte_t* packet) {
  using namespace hd::global;
  auto const _this{reinterpret_cast<DeadParser*>(user_data)};
  {
    std::scoped_lock queue_access(_this->mQueueLock);
    _this->mPacketQueue.emplace(pkthdr, packet);
  }
  _this->cv_consumer.notify_all();
}

void hd::type::DeadParser::consumer_job() {
  /// 采用标志变量keepRunning来控制detach的线程
  while (keepRunning) {
    std::unique_lock lock(this->mQueueLock);
    this->cv_consumer.wait(lock, [this] {
      return not this->mPacketQueue.empty() or not keepRunning;
    });
    if (not keepRunning) break;
    if (this->mPacketQueue.empty()) continue;
    raw_packet _raw_packet = this->mPacketQueue.front();
    this->mPacketQueue.pop();
    lock.unlock();
    {
      std::scoped_lock mtCvMtx(mProdLock);
      cv_producer.notify_one();
    }
    mSink->consumeData({_raw_packet});
  }
}

hd::type::DeadParser::~DeadParser() {
  /// 先等待游离worker线程消费队列直至为空
  while (not this->mPacketQueue.empty()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  /// 再控制游离线程停止访问主线程的资源
  keepRunning.store(false);
  cv_consumer.notify_all();
  /// 不要强制exit(0), 因为还有worker在死等。
  // exit(EXIT_SUCCESS);
}
