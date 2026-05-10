// 停等协议模拟程序
// 甲方发送数据包，乙方回送确认，通过随机丢包模拟自动重传机制

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

// 包结构体
struct Packet {
    int seq;      // 序列号
    char data[32];
    int ack_received;  // 是否收到确认
};

static const int PACKET_COUNT = 5;
static const double LOSS_RATE = 0.2;  // 20% 丢包率
static const int TIMEOUT_MS = 500;    // 超时时间

static std::mutex mtx;
static std::condition_variable cv;
static int sender_seq = -1;
static int receiver_ack = -1;
static volatile bool done = false;

// 模拟丢包
static bool should_drop() {
    return (rand() % 100) < (int)(LOSS_RATE * 100);
}

// 发送方线程
static void sender_thread_func() {
    std::cout << "[Sender] Thread started\n";
    
    for (int i = 0; i < PACKET_COUNT; ++i) {
        int retries = 0;
        const int max_retries = 3;
        
        while (retries < max_retries) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                sender_seq = i;
                std::cout << "[Sender] Sending packet seq=" << i << " (retry " << retries << ")\n";
            }
            
            // 模拟发送延迟和可能的丢包
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (should_drop()) {
                    std::cout << "[Sender] Packet seq=" << i << " was lost (randomly)\n";
                    retries++;
                    if (retries < max_retries) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));
                    }
                    continue;
                }
            }
            
            // 等待确认
            {
                std::unique_lock<std::mutex> lock(mtx);
                bool got_ack = false;
                auto start = std::chrono::steady_clock::now();
                
                while (true) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                    
                    if (receiver_ack == i) {
                        std::cout << "[Sender] Got ACK for seq=" << i << "\n";
                        got_ack = true;
                        break;
                    }
                    
                    if (elapsed > TIMEOUT_MS) {
                        std::cout << "[Sender] Timeout waiting ACK for seq=" << i << "\n";
                        break;
                    }
                    
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    lock.lock();
                }
                
                if (got_ack) {
                    break;
                }
            }
            
            retries++;
        }
        
        if (retries >= max_retries) {
            std::cout << "[Sender] Packet seq=" << i << " failed after " << max_retries << " retries\n";
        }
    }
    
    std::cout << "[Sender] All packets sent\n";
    {
        std::unique_lock<std::mutex> lock(mtx);
        done = true;
    }
    cv.notify_all();
}

// 接收方线程
static void receiver_thread_func() {
    std::cout << "[Receiver] Thread started\n";
    
    int expected_seq = 0;
    
    while (!done || expected_seq < PACKET_COUNT) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            
            if (sender_seq >= 0 && sender_seq == expected_seq) {
                // 模拟接收延迟
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                
                if (should_drop()) {
                    std::cout << "[Receiver] ACK for seq=" << sender_seq << " was lost\n";
                } else {
                    receiver_ack = sender_seq;
                    std::cout << "[Receiver] Sending ACK for seq=" << sender_seq << "\n";
                    expected_seq++;
                }
                sender_seq = -1;  // 清除以避免重复处理
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "[Receiver] Finished (all packets acknowledged)\n";
}

int main() {
    srand((unsigned int)time(NULL));
    
    std::cout << "===== 停等协议模拟 =====\n";
    std::cout << "丢包率: " << (int)(LOSS_RATE * 100) << "%\n";
    std::cout << "超时时间: " << TIMEOUT_MS << "ms\n\n";
    
    // 创建发送和接收线程
    std::thread sender(sender_thread_func);
    std::thread receiver(receiver_thread_func);
    
    // 等待两个线程完成
    sender.join();
    receiver.join();
    
    std::cout << "\n===== 模拟完成 =====\n";
    return 0;
}
