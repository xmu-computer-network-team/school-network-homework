// 网桥自学习功能模拟程序
// 网桥有两个网段，通过自学习建立MAC地址表

#include <iostream>
#include <map>

// 地址长度为4 bit，0xF表示广播
#define BROADCAST_ADDR 0xF

// MAC地址表项
struct MacTableEntry {
    int port;  // 端口号 (1或2)
};

// 网桥类
class Bridge {
private:
    std::map<int, MacTableEntry> mac_table;  // MAC地址表

public:
    // 学习源地址
    void learn(int src_addr, int src_port) {
        MacTableEntry entry;
        entry.port = src_port;
        mac_table[src_addr] = entry;
    }

    // 查找目的端口
    int lookup(int dest_addr) {
        if (dest_addr == BROADCAST_ADDR) {
            return 0;  // 0表示广播到所有端口
        }
        
        std::map<int, MacTableEntry>::iterator it = mac_table.find(dest_addr);
        if (it != mac_table.end()) {
            return it->second.port;
        }
        return -1;  // 未找到，需要泛洪
    }

    // 打印MAC地址表
    void print_table() {
        std::cout << "\n=== MAC地址表 ===" << std::endl;
        std::cout << "MAC地址\t端口" << std::endl;
        for (std::map<int, MacTableEntry>::iterator it = mac_table.begin(); 
             it != mac_table.end(); ++it) {
            std::cout << "0x" << std::hex << it->first 
                      << "\t" << std::dec << it->second.port << std::endl;
        }
    }
};

// 帧结构
struct Frame {
    int src_addr;   // 源地址
    int src_port;   // 源端口
    int dest_addr;  // 目的地址
};

// 处理一帧
void process_frame(Bridge& bridge, const Frame& frame) {
    std::cout << "\n处理帧: 源地址=0x" << std::hex << frame.src_addr 
              << ", 源端口=" << std::dec << frame.src_port 
              << ", 目的地址=0x" << std::hex << frame.dest_addr << std::endl;
    
    // 学习源地址
    bridge.learn(frame.src_addr, frame.src_port);
    std::cout << "学习: MAC地址 0x" << std::hex << frame.src_addr 
              << " -> 端口 " << std::dec << frame.src_port << std::endl;
    
    // 查找目的端口
    int dest_port = bridge.lookup(frame.dest_addr);
    
    if (frame.dest_addr == BROADCAST_ADDR) {
        std::cout << "目的端口: 广播到所有端口" << std::endl;
    } else if (dest_port == -1) {
        std::cout << "目的端口: 未知，泛洪到所有端口" << std::endl;
    } else if (dest_port == frame.src_port) {
        std::cout << "目的端口: " << dest_port << " (同网段，不转发)" << std::endl;
    } else {
        std::cout << "目的端口: " << dest_port << std::endl;
    }
}

int main() {
    Bridge bridge;
    
    std::cout << "===== 网桥自学习模拟程序 =====" << std::endl;
    
    // 测试帧序列
    Frame frames[] = {
        {0x1, 1, 0x2},  // 站点1(端口1)发送给站点2
        {0x2, 2, 0x1},  // 站点2(端口2)回复给站点1
        {0x3, 1, 0xF},  // 站点3(端口1)广播
        {0x4, 2, 0x3},  // 站点4(端口2)发送给站点3
        {0x1, 1, 0x4},  // 站点1发送给站点4
    };
    
    int frame_count = sizeof(frames) / sizeof(frames[0]);
    
    for (int i = 0; i < frame_count; ++i) {
        process_frame(bridge, frames[i]);
    }
    
    // 打印最终的MAC地址表
    bridge.print_table();
    
    return 0;
}
