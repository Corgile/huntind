//
// Created by brian on 3/13/24.
//

#ifndef HOUND_KAFKA_UTIL_HPP
#define HOUND_KAFKA_UTIL_HPP

#include <memory>
#include <librdkafka/rdkafkacpp.h>
#include <hound/sink/kafka/kafka_config.hpp>

namespace hd::util {
using namespace hd::type;

using namespace RdKafka;
using RdConfUptr = std::unique_ptr<Conf>;

void InitGetConf(hd::type::kafka_config const& conn_conf, RdConfUptr& _serverConf, RdConfUptr& _topic);

}
#endif //HOUND_KAFKA_UTIL_HPP
