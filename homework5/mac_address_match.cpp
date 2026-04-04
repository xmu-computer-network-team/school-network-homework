// 以太网网卡MAC地址匹配程序
// 判断帧是否应该被接收（本机地址、多播或广播）

#include <cstring>

// MAC地址长度
#define MAC_ADDRESS_LENGTH 6

// MAC地址类型
typedef unsigned char MAC_address[MAC_ADDRESS_LENGTH];

// 以太网帧结构体
struct EthernetFrame {
    MAC_address dest_mac;       // 目的地址
    MAC_address src_mac;        // 源地址
    unsigned short type;        // 类型/长度
    unsigned char data[1500];   // 数据
};

// 本机MAC地址
MAC_address this_mac_address = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

// 判断是否为广播地址 (FF:FF:FF:FF:FF:FF)
static int is_broadcast(const MAC_address addr) {
    for (int i = 0; i < MAC_ADDRESS_LENGTH; ++i) {
        if (addr[i] != 0xFF) {
            return 0;
        }
    }
    return 1;
}

// 判断是否为多播地址 (最低位为1)
static int is_multicast(const MAC_address addr) {
    // 多播地址: 第一个字节的最低位为1
    return (addr[0] & 0x01) ? 1 : 0;
}

// 判断是否为本机地址
static int is_this_mac(const MAC_address addr) {
    for (int i = 0; i < MAC_ADDRESS_LENGTH; ++i) {
        if (addr[i] != this_mac_address[i]) {
            return 0;
        }
    }
    return 1;
}

// MAC地址匹配函数
// 参数: frame - 以太网帧指针
// 返回值: 0 - 不匹配, 1 - 匹配
int mac_address_match(const struct EthernetFrame *frame) {
    if (!frame) {
        return 0;
    }

    // 检查是否为广播地址
    if (is_broadcast(frame->dest_mac)) {
        return 1;
    }

    // 检查是否为多播地址
    if (is_multicast(frame->dest_mac)) {
        return 1;
    }

    // 检查是否为本机地址
    if (is_this_mac(frame->dest_mac)) {
        return 1;
    }

    return 0;
}
