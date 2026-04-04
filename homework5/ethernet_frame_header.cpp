// 传统以太网帧头部定义
// 使用C语言定义

// MAC地址长度
#define MAC_ADDRESS_LENGTH 6

// MAC地址类型
typedef unsigned char MAC_address[MAC_ADDRESS_LENGTH];

// 传统以太网帧头部结构体
struct ethernet_frame_header {
    MAC_address dest_mac;       // 目的MAC地址 (6字节)
    MAC_address src_mac;        // 源MAC地址 (6字节)
    unsigned short type;        // 类型/长度字段 (2字节)
};
// 总长度: 14字节
