//
// Created by brian on 3/13/24.
//
#include "hound/common/util.hpp"
#include "hound/scope_guard.hpp"

void hd::util::SetFilter(pcap_handle_t& handle) {
  if (opt.filter.empty() or handle == nullptr) { return; }
  constexpr bpf_u_int32 net{0};
  bpf_program fp;
  scope_guard _guard([&fp] {
    pcap_freecode(&fp);
  });
  ELOG_DEBUG << "包过滤表达式: " << opt.filter;
  if (pcap_compile(handle.get(), &fp, opt.filter.c_str(), 0, net) == -1) {
    ELOG_ERROR << "解析 Filter 失败: " << opt.filter << pcap_geterr(handle.get());
    exit(EXIT_FAILURE);
  }
  if (pcap_setfilter(handle.get(), &fp) == -1) {
    ELOG_ERROR << "设置 Filter 失败: " << pcap_geterr(handle.get());
    exit(EXIT_FAILURE);
  }
  // pcap_freecode(&fp);
}

void hd::util::OpenLiveHandle(capture_option& option, pcap_handle_t& handle) {
  /* getFlowId device */
  if (option.device.empty()) {
    pcap_if_t* l;
    if (int32_t const rv{pcap_findalldevs(&l, ByteBuffer)}; rv == -1) {
      ELOG_ERROR << "找不到默认网卡设备" << ByteBuffer;
      exit(EXIT_FAILURE);
    }
    option.device = l->name;
    pcap_freealldevs(l);
  }
  ELOG_DEBUG << "网卡: " << option.device;
  /* open device */
  handle.reset(pcap_open_live(option.device.c_str(), BUFSIZ, 1, 1000, ByteBuffer));
  if (handle == nullptr) {
    ELOG_ERROR << "监听网卡设备失败: " << ByteBuffer;
    exit(EXIT_FAILURE);
  }
  SetFilter(handle);
  pcap_set_promisc(handle.get(), 1);
  pcap_set_buffer_size(handle.get(), 25 << 22);
}

void hd::util::Doc() {
  std::cout << "\t用法: \n";
  std::cout
    << "\t-J, --workers=1               处理流量包的线程数 (默认 1)\n"
    << "\t-F, --filter=\"filter\"         pcap filter (https://linux.die.net/man/7/pcap-filter)\n"
    << "                              " RED(
      "\t非常重要,必须设置并排除镜像流量服务器和kafka集群之间的流量,比如 \"not port 9092\"\n")
    << "\t-f, --fill=0                  空字节填充值 (默认 0)\n"
    << "\t    --cuda=0                  cuda设备 (默认 0)\n"
    << "\t-D, --duration=-1             D秒后结束抓包  (默认 -1, non-stop)\n"
    << "\t-N, --num=-1                  指定抓包的数量 (默认 -1, non-stop)\n"
    << "\t-E, --timeout=20              flow超时时间(新到达的packet距离上一个packet的时间) (默认 20)\n"
    // << "\t-K, --kafka-conf              kafka 配置文件路径\n"
    << "\t-M, --model                   torch script模型文件路径\n"
    << "\t-L, --min=10                  合并成流/json的时候，指定流的最 小 packet数量 (默认 10)\n"
    << "\t-R, --max=100                 合并成流/json的时候，指定流的最 大 packet数量 (默认 100)\n"
    << "\t-W, --write=/path/out         输出到文件, 需指定输出文件路径\n"
    << "\t-S, --stride=8                将 S 位二进制串转换为 uint 数值 (默认 8)\n"
    << "\t-p, --payload=0               包含 n 字节的 payload (默认 0)\n"
    << "\t    --pool=50                 kafka 连接池大小 (默认 50)\n"
    << "\t    --brokers=STRING          消息队列服务器地址\n"
    << "\t    --partition=1             消息队列分区数 (默认 1)\n"
    << "\t    --topic=STRING            消息队列 topic\n"
    << "\t    --sep=,                   csv列分隔符 (默认 ,)\n"
    << "\t-----------------" CYAN("以下选项不需要传入值")"----------------------------\n"
    << "\t-T, --timestamp               包含时间戳(秒,毫秒) (默认 不包含)\n"
    << "\t-C, --caplen                  包含报文长度 (默认 不包含)\n"
    << "\t-I, --index                   包含五元组 (默认 不包含)\n"
    << "\t-h, --help                    用法帮助\n"
    << std::endl;
}

void hd::util::ParseOptions(capture_option& arg, int argc, char** argv) {
  easylog::logger<>::instance();
  if (argc <= 1) [[unlikely]]{
    hd::util::Doc();
    exit(EXIT_SUCCESS);
  }
  int longind = 0, option, j;
  opterr = 0;
  while ((option = getopt_long(argc, argv, shortopts, longopts, &longind)) not_eq -1) {
    switch (option) {
    case 'd': arg.device = optarg;
      break;
    case 'D': arg.duration = std::stoi(optarg);
      break;
    case 'C': arg.include_pktlen = true;
      break;
    case 'F': arg.filter = optarg;
      break;
    case 'c': arg.num_gpus = std::stoi(optarg);
      break;
    case 'f': arg.fill_bit = std::stoi(optarg);
      break;
    case 'N': arg.num_packets = std::stoi(optarg);
      break;
    // case 'K': arg.kafka_config = optarg;
    //   if (arg.kafka_config.empty()) {
    //     ELOG_ERROR << "-K, --kafka-config 缺少值";
    //     exit(EXIT_FAILURE);
    //   }
    //   break;
    case 'p': arg.payload = std::stoi(optarg);
      break;
    case 'L': arg.min_packets = std::stoi(optarg);
      break;
    case 'R': arg.max_packets = std::stoi(optarg);
      break;
    case 'E': arg.flowTimeout = std::stoi(optarg);
      break;
    case 'T': arg.include_ts = true;
      break;
    case 'V': arg.verbose = true;
      break;
    case 'm': arg.separator.assign(optarg);
      std::sprintf(arg.format, "%s%s", "%ld", optarg);
      break;
    case 'B': arg.brokers.assign(optarg);
      break;
    case 'b': arg.poolSize = std::stoi(optarg);
      break;
    case 'x': arg.partition = std::stoi(optarg);
      break;
    case 'y': arg.topic.assign(optarg);
      break;
    case 'M': arg.model_path.assign(optarg);
      break;
    case 'I': arg.include_5tpl = true;
      break;
    case 'J': j = std::stoi(optarg);
      if (j < 1) {
        ELOG_ERROR << "worker 必须 >= 1";
        exit(EXIT_FAILURE);
      }
      arg.workers = j;
      ELOG_INFO << "Callback 线程: " << j;
      break;
    case 'S': arg.stride = std::stoi(optarg);
      if (arg.stride & arg.stride - 1 or arg.stride == 0) {
        ELOG_ERROR << "-S,  --stride 只能是1,2,4,8,16,32,64, 现在是: " << arg.stride;
        exit(EXIT_FAILURE);
      }
      break;
    case 'W': arg.write_file = true;
      arg.output_file = optarg;
      if (optarg == nullptr or arg.output_file.empty()) {
        ELOG_ERROR << "-W, --write 缺少值";
        exit(EXIT_FAILURE);
      }
      break;
    case '?': ELOG_ERROR << "选项 -" << static_cast<char>(optopt) << ":" RED(" 语法错误");
      ELOG_ERROR << "使用 -h, --help 查看使用方法";
      exit(EXIT_FAILURE);
    case 'h': Doc();
      exit(EXIT_SUCCESS);
    default: break;
    }
  }
}

bool hd::util::detail::_isTimeout(parsed_vector const& existing, parsed_packet const& _new) {
  if (existing.empty()) return false;
  return _new.mTsSec - existing.back().mTsSec >= opt.flowTimeout;
}

bool hd::util::detail::_isTimeout(parsed_vector const& existing) {
  long const now = hd::util::detail::timestampNow<std::chrono::seconds>();
  return now - existing.back().mTsSec >= opt.flowTimeout;
}

bool hd::util::detail::_checkLength(parsed_vector const& existing) {
  return existing.size() >= opt.min_packets and existing.size() <= opt.max_packets;
}

bool hd::util::IsFlowReady(parsed_vector const& existing, parsed_packet const& _new) {
  if (existing.size() == opt.max_packets) return true;
  return detail::_isTimeout(existing, _new) and detail::_checkLength(existing);
}
