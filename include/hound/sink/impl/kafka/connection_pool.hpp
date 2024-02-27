//
// Created by xhl on 6/20/23.
//

#ifndef HOUND_KAFKA_CONNECTION_POOL_HPP
#define HOUND_KAFKA_CONNECTION_POOL_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <hound/sink/impl/flow_check.hpp>

#include <hound/sink/impl/kafka/kafka_connection.hpp>

namespace hd::entity {
class connection_pool {
public:
  /// 获取连接池对象实例（懒汉式单例模式，在获取实例时才实例化对象）
  static connection_pool* create(const kafka_config& kafkaConfig) {
    static connection_pool* instance;
    if (instance == nullptr) {
      instance = new connection_pool(kafkaConfig);
    }
    return instance;
  }

  /**
   * 给外部提供接口，从连接池中获取一个可用的空闲连接.
   * 注意，这里不要直接返回指针，否则还需要定义一个（归还连接）的方法，还要自己去释放该指针。
   * 这里直接返回一个智能指针，出作用域后自动析构，（只需重定义析构即可--不释放而是归还）
   */
  std::shared_ptr<kafka_connection> get_connection() {
    std::unique_lock lock(_queueMutex);
    cv.wait_for(lock, std::chrono::seconds(_config.conn.timeout_sec),
                [&]()-> bool { return not _connectionQue.empty(); });
    if (_connectionQue.empty()) {
      std::printf("%s", "获取连接失败，等待空闲连接超时.\n");
      return nullptr;
    }
    std::shared_ptr<kafka_connection> connection(
      _connectionQue.front(),
      [&](kafka_connection* p_con)-> void {
        std::unique_lock _lock(_queueMutex);
        p_con->refreshAliveTime();
        _connectionQue.push(p_con);
      });
    _connectionQue.pop();
    cv.notify_all();
    return connection;
  }

  ~connection_pool() {
    cv.notify_all();
    _finished = true;
    while (not _connectionQue.empty()) {
      delete _connectionQue.front();
      _connectionQue.pop();
    }
    hd_debug(__PRETTY_FUNCTION__);
    {
      // ProducerDeliveryReportCb
      DeliveryReportCb* buff_1;
      EventCb* buff_2;
      PartitionerCb* buff_3;
      _serverConf->get(buff_1);
      _serverConf->get(buff_2);
      _topicConf->get(buff_3);

      delete buff_1;
      delete buff_2;
      delete buff_3;
    }
  }

private:
  connection_pool(const kafka_config& kafkaConfig) {
    this->_config = kafkaConfig;
    flow::InitGetConf(kafkaConfig.conn, _serverConf, _topicConf);
    for (int i = 0; i < _config.pool.init_size; ++i) {
      _connectionQue.emplace(new kafka_connection(_config.conn, _serverConf, _topicConf));
      ++_connectionCnt;
    }
    /// 启动一个新的线程，作为连接的生产者. 守护线程
    std::thread(&connection_pool::produceConnectionTask, this).detach();
    /// 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，并对其进行回收
    std::thread(&connection_pool::scannerConnectionTask, this).detach();
  }

  /// 运行在独立的线程中，专门负责生产新连接
  /// 非静态成员方法，其调用依赖对象，要把其设计为一个线程函数，需要绑定this指针。
  /// 把该线程函数写为类的成员方法，最大的好处是
  /// 非常方便访问当前对象的成员变量。（数据）
  void produceConnectionTask() {
    while (not _finished) {
      std::unique_lock lock(_queueMutex);
      cv.wait(lock, [&]()-> bool { return _connectionQue.empty() or _finished; });
      if (_finished) break;
      if (_connectionCnt < _config.pool.max_size) {
        _connectionQue.push(new kafka_connection(_config.conn, _serverConf, _topicConf));
        ++_connectionCnt;
      }
      cv.notify_all();
    }
    hd_debug("produceConnectionTask 结束");
  }

  /// 扫描超过maxIdleTime时间的空闲连接，进行对于连接的回收
  void scannerConnectionTask() {
    while (not _finished) {
      // 通过sleep实现定时
      for (int i = 0; i < _config.conn.max_idle; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (_finished) break;
      }
      // 扫描整个队列，释放多余的连接
      if (_finished) break;

      while (_connectionCnt > _config.pool.init_size) {
        std::scoped_lock lock(_queueMutex);
        kafka_connection* connection = _connectionQue.front();
        if (connection->getAliveTime() < _config.conn.max_idle * 1000) break;
        delete connection;
        _connectionQue.pop();
        --_connectionCnt;
      }
    }
    hd_debug("scannerConnectionTask 结束");
  }

  kafka_config _config;
  std::queue<kafka_connection*> _connectionQue;
  std::mutex _queueMutex;

  /// 记录connection连接的总数量
  std::atomic_int _connectionCnt{};

  /// 设置条件变量，用于连接生产线程和连接消费线程的通信
  std::condition_variable cv;

  std::atomic<bool> _finished{false};

  std::unique_ptr<Conf> _serverConf, _topicConf;
};
} // namespace xhl

#endif // HOUND_KAFKA_CONNECTION_POOL_HPP
