//
// Created by xhl on 6/20/23.
//

#ifndef HOUND_KAFKA_CONNECTION_POOL_HPP
#define HOUND_KAFKA_CONNECTION_POOL_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <hound/common/flow_check.hpp>

#include <hound/sink/impl/kafka/kafka_connection.hpp>

namespace hd::entity {

struct return_conn {
  std::mutex& mtx;
  std::vector<kafka_connection*>& que;

  return_conn(std::mutex& m, std::vector<kafka_connection*>& q)
    : mtx(m), que(q) {}

  void operator()(kafka_connection* p_con) const {
    std::unique_lock<std::mutex> lock(mtx);
    p_con->setInUse(false);
    p_con->resetIdleTime();
    que.push_back(p_con);
  }
};

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
                [&]() -> bool { return not _connectionQue.empty(); });
    if (_connectionQue.empty()) {
      std::printf("%s", "无空闲连接， 创建.\n");
      _connectionQue.emplace_back(new kafka_connection(_config.conn, _serverConf, _topicConf));
    }
    std::shared_ptr<kafka_connection> connection(_connectionQue.back(), return_conn(_queueMutex, _connectionQue));
    _connectionQue.pop_back();
    cv.notify_all();
    connection->setInUse(true);
    connection->resetIdleTime();
    return connection;
  }

  ~connection_pool() {
    cv.notify_all();
    _finished = true;
    std::for_each(
      _connectionQue.begin(),
      _connectionQue.end(),
      [&](kafka_connection* item) -> void {
        delete item;
      }
    );
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
    _connectionQue.reserve(_config.pool.max_size);
    for (int i = 0; i < _config.pool.init_size; ++i) {
      _connectionQue.emplace_back(new kafka_connection(_config.conn, _serverConf, _topicConf));
    }
    /// 连接的生产者.
    std::thread(&connection_pool::produceConnectionTask, this).detach();
    /// 连接回收
    std::thread(&connection_pool::clearIdleConnectionTask, this).detach();
  }

  void produceConnectionTask() {
    while (not _finished) {
      std::unique_lock lock(_queueMutex);
      cv.wait(lock, [&]() -> bool { return _connectionQue.empty() or _finished; });
      if (_finished) break;
      if (_connectionQue.size() < _config.pool.max_size) {
        _connectionQue.push_back(new kafka_connection(_config.conn, _serverConf, _topicConf));
      }
      cv.notify_all();
    }
    ELOG_DEBUG << YELLOW("函数 produceConnectionTask 结束");
  }

  /// 扫描超过maxIdleTime时间的空闲连接，进行对于连接的回收
  void clearIdleConnectionTask() {
    using namespace std::chrono_literals;
    while (not _finished) {
      for (int i = 0; i < _config.conn.max_idle; ++i) {
        std::this_thread::sleep_for(1s);
        if (_finished) break;
      }
      if (_finished) break;
      std::unique_lock lock(_queueMutex);
      for (auto it = _connectionQue.begin(); it not_eq _connectionQue.end();) {
        if (_connectionQue.size() <= _config.pool.init_size) break;
        if ((*it)->isRedundant()) {
          delete *it;
          it = _connectionQue.erase(it);
        } else it++;
      }
      lock.unlock();
    }
    ELOG_DEBUG << YELLOW("函数 clearIdleConnectionTask 结束");
  }

  kafka_config _config;
  std::vector<kafka_connection*> _connectionQue;
  std::mutex _queueMutex;

  /// 设置条件变量，用于连接生产线程和连接消费线程的通信
  std::condition_variable cv;

  std::atomic<bool> _finished{false};

  std::unique_ptr<Conf> _serverConf, _topicConf;
};
} // namespace xhl

#endif // HOUND_KAFKA_CONNECTION_POOL_HPP
