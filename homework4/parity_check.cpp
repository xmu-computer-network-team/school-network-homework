// 使用偶校验方式：所有位中1的个数应为偶数

// 将字节转换为位值（0或1）
static unsigned char to_bit(unsigned char x) {
    return x ? 1 : 0;
}

// 统计消息中1的个数
static int count_ones(const unsigned char *msg, const int msg_length) {
    int count = 0;
    for (int i = 0; i < msg_length; ++i) {
        count += to_bit(msg[i]);
    }
    return count;
}

// 奇偶校验函数
// 参数: msg - 消息数组（最后一个元素为校验位）
//       msg_length - 消息长度（包括校验位）
// 返回值: 0 - 校验失败, 1 - 校验通过
int parity_check(const unsigned char *msg, const int msg_length) {
    // 参数检查
    if (!msg || msg_length <= 0) {
        return 0;
    }

    // 统计所有位中1的个数
    int ones = count_ones(msg, msg_length);

    // 偶校验：1的个数应为偶数
    if (ones % 2 == 0) {
        return 1;  // 校验通过
    } else {
        return 0;  // 校验失败
    }
}
