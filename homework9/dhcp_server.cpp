#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <map>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

// 第50题：
// 一个简单的 DHCP 服务器，给同一子网内的客户端分配固定 IP。
// 这里实现最小可用的 DHCPDISCOVER / DHCPREQUEST 响应流程。

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_FIXED_IP "192.168.1.2"
#define DHCP_NETMASK "255.255.255.0"
#define DHCP_ROUTER "192.168.1.1"
#define DHCP_DNS "8.8.8.8"

#define DHCP_MAGIC_COOKIE 0x63825363
#define DHCP_OP_BOOTREQUEST 1
#define DHCP_OP_BOOTREPLY 2
#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET 6
#define DHCP_MSG_DISCOVER 1
#define DHCP_MSG_OFFER 2
#define DHCP_MSG_REQUEST 3
#define DHCP_MSG_DECLINE 4
#define DHCP_MSG_ACK 5
#define DHCP_MSG_NAK 6

#define DHCP_OPT_PAD 0
#define DHCP_OPT_SUBNET_MASK 1
#define DHCP_OPT_ROUTER 3
#define DHCP_OPT_DNS 6
#define DHCP_OPT_REQUESTED_IP 50
#define DHCP_OPT_LEASE_TIME 51
#define DHCP_OPT_MSG_TYPE 53
#define DHCP_OPT_SERVER_ID 54
#define DHCP_OPT_PARAM_LIST 55
#define DHCP_OPT_END 255

#pragma pack(push, 1)
struct dhcp_packet {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint32_t magic_cookie;
    uint8_t options[312];
};
#pragma pack(pop)

struct DhcpLease {
    uint32_t ip;
    uint8_t mac[6];
};

static uint32_t parse_ipv4(const char *ip) {
    in_addr addr;
    std::memset(&addr, 0, sizeof(addr));
    if (inet_pton(AF_INET, ip, &addr) != 1) {
        return 0;
    }
    return addr.s_addr;
}

static void copy_mac(uint8_t *dst, const uint8_t *src) {
    for (int i = 0; i < 6; ++i) {
        dst[i] = src[i];
    }
}

static bool mac_equal(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < 6; ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

static uint8_t get_dhcp_message_type(const dhcp_packet &pkt) {
    for (int i = 0; i < (int)sizeof(pkt.options);) {
        uint8_t code = pkt.options[i];
        if (code == DHCP_OPT_END) break;
        if (code == DHCP_OPT_PAD) {
            ++i;
            continue;
        }
        if (i + 1 >= (int)sizeof(pkt.options)) break;
        uint8_t len = pkt.options[i + 1];
        if (code == DHCP_OPT_MSG_TYPE && len >= 1) {
            return pkt.options[i + 2];
        }
        i += 2 + len;
    }
    return 0;
}

static uint32_t get_requested_ip(const dhcp_packet &pkt) {
    for (int i = 0; i < (int)sizeof(pkt.options);) {
        uint8_t code = pkt.options[i];
        if (code == DHCP_OPT_END) break;
        if (code == DHCP_OPT_PAD) {
            ++i;
            continue;
        }
        if (i + 1 >= (int)sizeof(pkt.options)) break;
        uint8_t len = pkt.options[i + 1];
        if (code == DHCP_OPT_REQUESTED_IP && len == 4) {
            uint32_t ip;
            std::memcpy(&ip, &pkt.options[i + 2], 4);
            return ip;
        }
        i += 2 + len;
    }
    return 0;
}

static int write_option(uint8_t *opt, int pos, uint8_t code, const void *data, uint8_t len) {
    opt[pos++] = code;
    opt[pos++] = len;
    std::memcpy(opt + pos, data, len);
    return pos + len;
}

static int write_message_type(uint8_t *opt, int pos, uint8_t type) {
    return write_option(opt, pos, DHCP_OPT_MSG_TYPE, &type, 1);
}

static int build_reply(dhcp_packet &reply, const dhcp_packet &req, uint8_t msg_type, uint32_t yiaddr, uint32_t server_ip) {
    std::memset(&reply, 0, sizeof(reply));
    reply.op = DHCP_OP_BOOTREPLY;
    reply.htype = DHCP_HTYPE_ETHERNET;
    reply.hlen = DHCP_HLEN_ETHERNET;
    reply.hops = 0;
    reply.xid = req.xid;
    reply.secs = req.secs;
    reply.flags = req.flags;
    reply.ciaddr = 0;
    reply.yiaddr = yiaddr;
    reply.siaddr = server_ip;
    reply.giaddr = req.giaddr;
    std::memcpy(reply.chaddr, req.chaddr, sizeof(reply.chaddr));
    reply.magic_cookie = htonl(DHCP_MAGIC_COOKIE);

    int pos = 0;
    pos = write_message_type(reply.options, pos, msg_type);

    uint32_t mask = parse_ipv4(DHCP_NETMASK);
    uint32_t router = parse_ipv4(DHCP_ROUTER);
    uint32_t dns = parse_ipv4(DHCP_DNS);
    uint32_t lease = htonl(3600);

    pos = write_option(reply.options, pos, DHCP_OPT_SUBNET_MASK, &mask, 4);
    pos = write_option(reply.options, pos, DHCP_OPT_ROUTER, &router, 4);
    pos = write_option(reply.options, pos, DHCP_OPT_DNS, &dns, 4);
    pos = write_option(reply.options, pos, DHCP_OPT_LEASE_TIME, &lease, 4);
    pos = write_option(reply.options, pos, DHCP_OPT_SERVER_ID, &server_ip, 4);

    reply.options[pos++] = DHCP_OPT_END;
    return sizeof(dhcp_packet);
}

static void print_mac(const uint8_t *mac) {
    std::printf("%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::printf("WSAStartup failed\n");
        return 1;
    }
#endif

    int sockfd = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        std::perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes));
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&yes, sizeof(yes));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DHCP_SERVER_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        std::perror("bind");
#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        close(sockfd);
#endif
        return 1;
    }

    uint32_t server_ip = parse_ipv4(DHCP_ROUTER);
    uint32_t fixed_ip = parse_ipv4(DHCP_FIXED_IP);

    std::map<std::string, DhcpLease> lease_table;

    std::printf("DHCP server started. Fixed IP = %s\n", DHCP_FIXED_IP);

    unsigned char buf[1500];
    while (true) {
        sockaddr_in client_addr;
#ifdef _WIN32
        int client_len = sizeof(client_addr);
#else
        socklen_t client_len = sizeof(client_addr);
#endif
        std::memset(&client_addr, 0, sizeof(client_addr));

        int n = (int)recvfrom(sockfd, (char *)buf, sizeof(buf), 0, (sockaddr *)&client_addr, &client_len);
        if (n < (int)sizeof(dhcp_packet)) {
            continue;
        }

        dhcp_packet req;
        std::memcpy(&req, buf, sizeof(dhcp_packet));
        if (ntohl(req.magic_cookie) != DHCP_MAGIC_COOKIE) {
            continue;
        }

        uint8_t msg_type = get_dhcp_message_type(req);
        uint32_t requested_ip = get_requested_ip(req);

        char mac_key[32];
        std::snprintf(mac_key, sizeof(mac_key), "%02X:%02X:%02X:%02X:%02X:%02X",
                      req.chaddr[0], req.chaddr[1], req.chaddr[2],
                      req.chaddr[3], req.chaddr[4], req.chaddr[5]);

        DhcpLease &lease = lease_table[mac_key];
        if (lease.ip == 0 || !mac_equal(lease.mac, req.chaddr)) {
            lease.ip = fixed_ip;
            copy_mac(lease.mac, req.chaddr);
        }

        dhcp_packet reply;
        if (msg_type == DHCP_MSG_DISCOVER) {
            build_reply(reply, req, DHCP_MSG_OFFER, lease.ip, server_ip);
            std::printf("DISCOVER from %s -> OFFER %s\n", mac_key, DHCP_FIXED_IP);
        } else if (msg_type == DHCP_MSG_REQUEST) {
            uint32_t chosen = (requested_ip != 0) ? requested_ip : lease.ip;
            if (chosen == 0) {
                chosen = lease.ip;
            }
            build_reply(reply, req, DHCP_MSG_ACK, chosen, server_ip);
            std::printf("REQUEST from %s -> ACK %s\n", mac_key, DHCP_FIXED_IP);
        } else {
            continue;
        }

        sockaddr_in broadcast_addr;
        std::memset(&broadcast_addr, 0, sizeof(broadcast_addr));
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_port = htons(DHCP_CLIENT_PORT);
        broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;

        sendto(sockfd, (const char *)&reply, sizeof(reply), 0, (sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
    }

#ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif
    return 0;
}
