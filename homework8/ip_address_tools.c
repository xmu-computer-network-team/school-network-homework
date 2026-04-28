#include <stddef.h>

// 第59题:
// 判断 ip 是否属于 netip/mask 指定的网络。
// 返回 1 表示匹配，0 表示不匹配或参数无效。
int is_in_net(unsigned char *ip, unsigned char *netip, unsigned char *mask) {
    if (ip == NULL || netip == NULL || mask == NULL) {
        return 0;
    }

    for (int i = 0; i < 4; ++i) {
        unsigned char ip_net = (unsigned char)(ip[i] & mask[i]);
        unsigned char net = (unsigned char)(netip[i] & mask[i]);
        if (ip_net != net) {
            return 0;
        }
    }

    return 1;
}

// 第60题:
// 传统分类地址 A-E 映射为 0-4。
// A: 0xxxxxxx -> 0
// B: 10xxxxxx -> 1
// C: 110xxxxx -> 2
// D: 1110xxxx -> 3
// E: 1111xxxx -> 4
int classwise(unsigned char *ip) {
    if (ip == NULL) {
        return -1;
    }

    unsigned char first = ip[0];

    if ((first & 0x80) == 0x00) {
        return 0;
    }
    if ((first & 0xC0) == 0x80) {
        return 1;
    }
    if ((first & 0xE0) == 0xC0) {
        return 2;
    }
    if ((first & 0xF0) == 0xE0) {
        return 3;
    }

    return 4;
}
