#include <csignal>
#include <hound/common/util.hpp>
#include <hound/common/global.hpp>

#include <hound/parser/dead_parser.hpp>
#include <hound/csv-helper.hpp>

namespace hd::global {
type::capture_option opt;
std::string fillBit;
std::unordered_map<std::string, bool> attacks = {};
}

void static InitDict() {
  io::CSVReader<6> in(hd::global::opt.csv);
  in.read_header(io::ignore_extra_column,
                 "Source IP", "Source Port",
                 "Destination IP", "Destination Port",
                 "Protocol", "Label");

  std::string SourceIP;
  std::string SourcePort;
  std::string DestinationIP;
  std::string DestinationPort;
  std::string Protocol;
  std::string Label;
  while (in.read_row(SourceIP, SourcePort,
                     DestinationIP, DestinationPort,
                     Protocol, Label)) {
    // do stuff with the data
    std::stringstream key;
    key
      << SourceIP << "-" << DestinationIP << "-"
      << SourcePort << "-"
      << DestinationPort << "-" << Protocol;
    // 相信数据集对于同样的flowID, 其label不会不同， 所以直接覆盖
//    hd::global::id_type[std::move(key.str())].assign(Label);
  }
}

static void initdictbypcap() {
    auto callback = [](const u_char* packet, struct timeval ts, unsigned int capture_len) {
        struct tcphdr* tcp;
        struct udphdr* udp;
        constexpr int ethernet_header_length = 14; // Length of Ethernet header
        char five_tuple[100]; // Buffer for five-tuple string
        // Parse the Ethernet header
        const struct ether_header* eth_header = (struct ether_header*)packet;
        int type = ntohs(eth_header->ether_type);

        // Check if the packet contains a VLAN tag
        if (type == ETHERTYPE_VLAN) {
            constexpr int vlan_header_length = 4;
            // Adjust for VLAN tag
            type = ntohs(*(uint16_t*)(packet + 16));
            packet += vlan_header_length;
        }

        if (type != ETHERTYPE_IP) {
            // Not an IP packet
            return;
        }

        // Parse the IP header
        packet += ethernet_header_length;
        auto ip = (struct ip*)packet;
        unsigned int IP_header_length = ip->ip_hl * 4; // IP header length

        // Ensure the packet is long enough
        if (capture_len < ethernet_header_length + IP_header_length) return;

        snprintf(five_tuple, sizeof(five_tuple), "%s_%s_", inet_ntoa(ip->ip_src), inet_ntoa(ip->ip_dst));

        switch (ip->ip_p) {
            case IPPROTO_TCP: tcp = (struct tcphdr*)(packet + IP_header_length);
                snprintf(five_tuple + strlen(five_tuple), sizeof(five_tuple) - strlen(five_tuple),
                         "%d_%d_TCP", ntohs(tcp->th_sport), ntohs(tcp->th_dport));
                break;
            case IPPROTO_UDP: udp = (struct udphdr*)(packet + IP_header_length);
                snprintf(five_tuple + strlen(five_tuple), sizeof(five_tuple) - strlen(five_tuple),
                         "%d_%d_UDP", ntohs(udp->uh_sport), ntohs(udp->uh_dport));
                break;
            default: snprintf(five_tuple + strlen(five_tuple), sizeof(five_tuple) - strlen(five_tuple), "Other Protocol");
                break;
        }
        hd::global::attacks[five_tuple] = true;
    };
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_pkthdr header{};
    const u_char* packet;
    const auto filename = "/data/LocalSend/all_attack.pcapng";
    pcap_t* handle = pcap_open_offline(filename, errbuf);
    if (handle == nullptr) {
        fprintf(stderr, "Couldn't open pcap file %s: %s\n", filename, errbuf);
        return;
    }
    while ((packet = pcap_next(handle, &header)) != nullptr) {
        callback(packet, header.ts, header.caplen);
    }
    pcap_close(handle);
}


int main(const int argc, char* argv[]) {
  using namespace hd::global;
  using namespace hd::type;
  hd::util::ParseOptions(opt, argc, argv);
  initdictbypcap();
    std::printf("%s, attacks = %lu\n", "ok", attacks.size());
  static std::unique_ptr<DeadParser> deadParser{nullptr};
  deadParser = std::make_unique<DeadParser>();
  deadParser->processFile();
  return 0;
}
