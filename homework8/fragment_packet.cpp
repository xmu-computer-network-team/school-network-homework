#include <vector>

struct Fragment {
    int length;  // 分片总长度（含20字节首部）
    int offset;  // 分片偏移（单位: 8字节）
};

// 第61题:
// packetLength: 原始报文总长度（含20字节首部）
// pathMTUs: 路径上各跳的 MTU
// 返回每个分片的长度和偏移。
std::vector<Fragment> fragmentPacket(int packetLength, const std::vector<int>& pathMTUs) {
    std::vector<Fragment> result;

    const int headerLen = 20;
    if (packetLength <= headerLen || pathMTUs.empty()) {
        return result;
    }

    // 路径最小 MTU 决定可用最大分片大小
    int minMTU = pathMTUs[0];
    for (int i = 1; i < (int)pathMTUs.size(); ++i) {
        if (pathMTUs[i] < minMTU) {
            minMTU = pathMTUs[i];
        }
    }

    // MTU 小于等于首部，无法承载有效数据
    if (minMTU <= headerLen) {
        return result;
    }

    int totalData = packetLength - headerLen;
    int maxDataPerFragment = minMTU - headerLen;

    // 除最后一片外，数据长度应为8字节整数倍
    int alignedData = (maxDataPerFragment / 8) * 8;
    if (alignedData == 0) {
        return result;
    }

    int sentData = 0;
    while (totalData - sentData > alignedData) {
        Fragment frag;
        frag.length = headerLen + alignedData;
        frag.offset = sentData / 8;
        result.push_back(frag);
        sentData += alignedData;
    }

    // 最后一片可以不是8字节整数倍
    int lastData = totalData - sentData;
    if (lastData > 0) {
        Fragment last;
        last.length = headerLen + lastData;
        last.offset = sentData / 8;
        result.push_back(last);
    }

    return result;
}
