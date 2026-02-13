#include "NetworkManager.h"
#include <iostream>
#include <vector>

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#endif

#define MDNS_IMPLEMENTATION
#include "mdns.h"

struct mDNSRecords {
    std::string service_type;
    std::string instance_name;
    std::string hostname;
    std::string local_ip;
    uint16_t port;
};

NetworkManager::NetworkManager(uint16_t port) : servicePort(port) {}

NetworkManager::~NetworkManager() {
    stop();
}

void NetworkManager::start() {
    if (running) return;
    running = true;
    mDNSThread = std::thread(&NetworkManager::runmDNS, this);
}

void NetworkManager::stop() {
    running = false;
    if (mDNSThread.joinable()) {
        mDNSThread.join();
    }
}

static std::string get_local_ip() {
    std::string ip = "127.0.0.1";
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) return ip;

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) continue;
        char host[NI_MAXHOST];
        if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST) == 0) {
            std::string current_ip(host);
            if (current_ip != "127.0.0.1") {
                ip = current_ip;
                break;
            }
        }
    }
    freeifaddrs(ifaddr);
    return ip;
}

static bool compare_names(const std::string& a, const std::string& b) {
    if (a == b) return true;
    if (a.length() > b.length() && a.back() == '.' && a.substr(0, a.length() - 1) == b) return true;
    if (b.length() > a.length() && b.back() == '.' && b.substr(0, b.length() - 1) == a) return true;
    return false;
}

static int query_callback(int sock, const struct sockaddr* from, size_t addrlen, mdns_entry_type_t entry,
                         uint16_t query_id, uint16_t rtype, uint16_t rclass, uint32_t ttl, const void* data,
                         size_t size, size_t name_offset, size_t name_length, size_t record_offset,
                         size_t record_length, void* user_data) {
    if (entry != MDNS_ENTRYTYPE_QUESTION) return 0;

    auto* records = static_cast<mDNSRecords*>(user_data);
    char name_buffer[256];
    mdns_string_t name_str = mdns_string_extract(data, size, &name_offset, name_buffer, sizeof(name_buffer));
    
    std::string queried_name(name_str.str, name_str.length);
    std::cout << "[Network] Received query for: " << queried_name << " (Type: " << rtype << ")" << std::endl;

    // Respond if the query matches our service type, instance name, or hostname
    if (compare_names(queried_name, records->service_type) || 
        compare_names(queried_name, records->instance_name) ||
        compare_names(queried_name, records->hostname)) {
        
        std::cout << "[Network] Sending response for: " << queried_name << std::endl;
        mdns_record_t ptr_record = {};
        ptr_record.name = {records->service_type.c_str(), records->service_type.length()};
        ptr_record.type = MDNS_RECORDTYPE_PTR;
        ptr_record.data.ptr.name = {records->instance_name.c_str(), records->instance_name.length()};

        mdns_record_t srv_record = {};
        srv_record.name = {records->instance_name.c_str(), records->instance_name.length()};
        srv_record.type = MDNS_RECORDTYPE_SRV;
        srv_record.data.srv.name = {records->hostname.c_str(), records->hostname.length()};
        srv_record.data.srv.port = records->port;

        mdns_record_t txt_record = {};
        txt_record.name = {records->instance_name.c_str(), records->instance_name.length()};
        txt_record.type = MDNS_RECORDTYPE_TXT;
        txt_record.data.txt.key = {nullptr, 0};
        txt_record.data.txt.value = {nullptr, 0};

        mdns_record_t a_record = {};
        a_record.name = {records->hostname.c_str(), records->hostname.length()};
        a_record.type = MDNS_RECORDTYPE_A;
        a_record.data.a.addr.sin_family = AF_INET;
        inet_pton(AF_INET, records->local_ip.c_str(), &a_record.data.a.addr.sin_addr);

        mdns_record_t additional[3] = { srv_record, txt_record, a_record };
        
        void* buffer = malloc(2048);
        if (compare_names(queried_name, records->hostname)) {
            // If they asked specifically for the hostname, give them the A record as the main answer
            mdns_query_answer_multicast(sock, buffer, 2048, a_record, nullptr, 0, nullptr, 0);
        } else {
            // Otherwise give the PTR record and everything else as additional
            mdns_announce_multicast(sock, buffer, 2048, ptr_record, nullptr, 0, additional, 3);
        }
        free(buffer);
    }
    return 0;
}

void NetworkManager::runmDNS() {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(5353);

    int sock = mdns_socket_open_ipv4(&addr);
    if (sock < 0) {
        std::cerr << "[Network] Error: Could not open mDNS socket. Is another mDNS responder (like avahi-daemon) running?" << std::endl;
        return;
    }

    mDNSRecords records;
    records.service_type = "_hockey-score._tcp.local.";
    records.instance_name = "Hockey Scoreboard." + records.service_type;
    records.port = servicePort;
    records.local_ip = get_local_ip();

    char hostname_buf[256];
    gethostname(hostname_buf, sizeof(hostname_buf));
    std::string host_str(hostname_buf);
    if (host_str.find(".local") == std::string::npos) {
        records.hostname = host_str + ".local.";
    } else if (host_str.back() != '.') {
        records.hostname = host_str + ".";
    } else {
        records.hostname = host_str;
    }

    std::cout << "[Network] mDNS Responder active" << std::endl;
    std::cout << "  Instance: " << records.instance_name << std::endl;
    std::cout << "  Hostname: " << records.hostname << std::endl;
    std::cout << "  IP:       " << records.local_ip << std::endl;
    std::cout << "  Port:     " << records.port << std::endl;

    void* buffer = malloc(2048);
    int check_counter = 0;

    while (running) {
        // Listen for queries (non-blocking-ish)
        mdns_socket_listen(sock, buffer, 2048, query_callback, &records);

        // Every ~2 seconds (since listen has a timeout), send an unsolicited announcement
        if (++check_counter >= 20) { // Assuming listen loop is fast
            mdns_record_t ptr_record = {};
            ptr_record.name = {records.service_type.c_str(), records.service_type.length()};
            ptr_record.type = MDNS_RECORDTYPE_PTR;
            ptr_record.data.ptr.name = {records.instance_name.c_str(), records.instance_name.length()};

            mdns_record_t srv_record = {};
            srv_record.name = {records.instance_name.c_str(), records.instance_name.length()};
            srv_record.type = MDNS_RECORDTYPE_SRV;
            srv_record.data.srv.name = {records.hostname.c_str(), records.hostname.length()};
            srv_record.data.srv.port = records.port;

            mdns_record_t a_record = {};
            a_record.name = {records.hostname.c_str(), records.hostname.length()};
            a_record.type = MDNS_RECORDTYPE_A;
            a_record.data.a.addr.sin_family = AF_INET;
            inet_pton(AF_INET, records.local_ip.c_str(), &a_record.data.a.addr.sin_addr);

            mdns_record_t txt_record = {};
            txt_record.name = {records.instance_name.c_str(), records.instance_name.length()};
            txt_record.type = MDNS_RECORDTYPE_TXT;
            txt_record.data.txt.key = {nullptr, 0};
            txt_record.data.txt.value = {nullptr, 0};

            mdns_record_t additional[3] = { srv_record, txt_record, a_record };
            mdns_announce_multicast(sock, buffer, 2048, ptr_record, nullptr, 0, additional, 3);
            check_counter = 0;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    free(buffer);
    mdns_socket_close(sock);
}
