#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#endif

// 第49题：
// 一个简单的 ICMP Ping 客户端/服务器程序。
// server 模式: 接收 ICMP Echo Request 并回 Echo Reply。
// client 模式: 发送一次 Echo Request，并等待 Reply。

static unsigned short icmp_checksum(const unsigned short *buf, int len) {
    unsigned int sum = 0;
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    if (len == 1) {
        sum += *(const unsigned char *)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

static int create_raw_icmp_socket() {
#ifdef _WIN32
    return (int)socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
#else
    return socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
#endif
}

static void print_ip(const sockaddr_in &addr) {
    char ipbuf[64];
    const char *p = inet_ntop(AF_INET, (void *)&addr.sin_addr, ipbuf, sizeof(ipbuf));
    if (p) {
        std::printf("%s", p);
    } else {
        std::printf("unknown");
    }
}

static void run_server() {
    int sockfd = create_raw_icmp_socket();
    if (sockfd < 0) {
        std::perror("socket");
        return;
    }

    std::printf("ICMP server started. Waiting for echo requests...\n");

    unsigned char buf[1500];
    while (true) {
        sockaddr_in src_addr;
#ifdef _WIN32
        int addr_len = sizeof(src_addr);
#else
        socklen_t addr_len = sizeof(src_addr);
#endif
        std::memset(&src_addr, 0, sizeof(src_addr));

        int n = (int)recvfrom(sockfd, (char *)buf, sizeof(buf), 0, (sockaddr *)&src_addr, &addr_len);
        if (n <= 0) {
            continue;
        }

        int ip_header_len = (buf[0] & 0x0F) * 4;
        if (n < ip_header_len + 8) {
            continue;
        }

        struct icmphdr *icmp = (struct icmphdr *)(buf + ip_header_len);
        if (icmp->type != ICMP_ECHO) {
            continue;
        }

        std::printf("recv echo request from ");
        print_ip(src_addr);
        std::printf(" id=%u seq=%u\n", ntohs(icmp->un.echo.id), ntohs(icmp->un.echo.sequence));

        icmp->type = ICMP_ECHOREPLY;
        icmp->checksum = 0;
        icmp->checksum = icmp_checksum((unsigned short *)icmp, n - ip_header_len);

        int sent = (int)sendto(sockfd, (const char *)icmp, n - ip_header_len, 0, (sockaddr *)&src_addr, addr_len);
        if (sent > 0) {
            std::printf("send echo reply to ");
            print_ip(src_addr);
            std::printf(" ok\n");
        }
    }

#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}

static int resolve_host(const char *host, sockaddr_in &addr) {
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    unsigned long ip = inet_addr(host);
    if (ip != INADDR_NONE) {
        addr.sin_addr.s_addr = ip;
        return 0;
    }

    hostent *hp = gethostbyname(host);
    if (!hp) {
        return -1;
    }
    std::memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
    return 0;
}

static void run_client(const char *host) {
    int sockfd = create_raw_icmp_socket();
    if (sockfd < 0) {
        std::perror("socket");
        return;
    }

    sockaddr_in dest_addr;
    if (resolve_host(host, dest_addr) != 0) {
        std::printf("resolve host failed: %s\n", host);
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return;
    }

    unsigned char packet[64];
    std::memset(packet, 0, sizeof(packet));

    struct icmphdr *icmp = (struct icmphdr *)packet;
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = htons(1234);
    icmp->un.echo.sequence = htons(1);

    const char *payload = "hello icmp";
    std::memcpy(packet + sizeof(struct icmphdr), payload, std::strlen(payload));
    int packet_len = (int)(sizeof(struct icmphdr) + std::strlen(payload));
    icmp->checksum = 0;
    icmp->checksum = icmp_checksum((unsigned short *)icmp, packet_len);

    std::printf("send echo request to %s\n", host);
    int sent = (int)sendto(sockfd, (const char *)packet, packet_len, 0, (sockaddr *)&dest_addr, sizeof(dest_addr));
    if (sent <= 0) {
        std::perror("sendto");
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return;
    }

    unsigned char reply[1500];
#ifdef _WIN32
    int addr_len = sizeof(sockaddr_in);
#else
    socklen_t addr_len = sizeof(sockaddr_in);
#endif
    sockaddr_in from_addr;
    std::memset(&from_addr, 0, sizeof(from_addr));

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET((unsigned int)sockfd, &readfds);
    timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    int ret = select(sockfd + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0) {
        std::printf("timeout waiting for reply\n");
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return;
    }

    int n = (int)recvfrom(sockfd, (char *)reply, sizeof(reply), 0, (sockaddr *)&from_addr, &addr_len);
    if (n <= 0) {
        std::perror("recvfrom");
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return;
    }

    int ip_header_len = (reply[0] & 0x0F) * 4;
    if (n < ip_header_len + 8) {
        std::printf("bad reply\n");
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return;
    }

    struct icmphdr *r_icmp = (struct icmphdr *)(reply + ip_header_len);
    if (r_icmp->type == ICMP_ECHOREPLY) {
        std::printf("recv echo reply from ");
        print_ip(from_addr);
        std::printf(" id=%u seq=%u\n", ntohs(r_icmp->un.echo.id), ntohs(r_icmp->un.echo.sequence));
    } else {
        std::printf("recv non-echo reply packet\n");
    }

#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::printf("WSAStartup failed\n");
        return 1;
    }
#endif

    if (argc < 2) {
        std::printf("usage: %s server | client <host>\n", argv[0]);
#ifdef _WIN32
        WSACleanup();
#endif
        return 0;
    }

    if (std::strcmp(argv[1], "server") == 0) {
        run_server();
    } else if (std::strcmp(argv[1], "client") == 0) {
        const char *host = (argc >= 3) ? argv[2] : "127.0.0.1";
        run_client(host);
    } else {
        std::printf("unknown mode: %s\n", argv[1]);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
