// 滑动窗口机制模拟程序
// 用线程模拟双方通信，输出窗口各部分的状态

#include <iostream>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <chrono>

static const int SEND_WINDOW_SIZE = 4;
static const int RECV_WINDOW_SIZE = 4;
static const int TOTAL_PACKETS = 10;
static const double LOSS_RATE = 0.15;  // 15% 丢包率

// 滑动窗口状态
struct SenderWindow {
    int base;              // 已发送并收到确认的第一个包
    int next_seq;          // 下一个要发送的包
    int* ack_received;     // 各包是否收到ACK
    int sent_count;        // 已发送数
    int acked_count;       // 已确认数
};

struct ReceiverWindow {
    int base;              // 允许接收的第一个包
    int* received;         // 各包是否已接收
    int received_count;    // 已接收的包数
};

static SenderWindow sender_win = {0, 0, nullptr, 0, 0};
static ReceiverWindow receiver_win = {0, nullptr, 0};
static std::mutex mtx;
static volatile bool done = false;
static int global_clock = 0;

static void init_windows() {
    sender_win.ack_received = new int[TOTAL_PACKETS];
    receiver_win.received = new int[TOTAL_PACKETS];
    
    for (int i = 0; i < TOTAL_PACKETS; ++i) {
        sender_win.ack_received[i] = 0;
        receiver_win.received[i] = 0;
    }
}

static void cleanup_windows() {
    delete[] sender_win.ack_received;
    delete[] receiver_win.received;
}

static bool should_drop() {
    return (rand() % 100) < (int)(LOSS_RATE * 100);
}

static void sender_thread_func() {
    while (sender_win.acked_count < TOTAL_PACKETS) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            
            // 发送窗口内未发送的包
            while (sender_win.next_seq < sender_win.base + SEND_WINDOW_SIZE &&
                   sender_win.next_seq < TOTAL_PACKETS) {
                if (!should_drop()) {
                    std::cout << "[Sender] Sending packet " << sender_win.next_seq << "\n";
                } else {
                    std::cout << "[Sender] Packet " << sender_win.next_seq << " dropped\n";
                }
                sender_win.next_seq++;
                sender_win.sent_count++;
            }
            
            // 处理接收到的ACK
            for (int i = sender_win.base; i < TOTAL_PACKETS; ++i) {
                if (sender_win.ack_received[i] && i == sender_win.base) {
                    sender_win.base++;
                    sender_win.acked_count++;
                } else if (!sender_win.ack_received[i]) {
                    break;
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

static void receiver_thread_func() {
    while (receiver_win.received_count < TOTAL_PACKETS) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            
            // 模拟接收包并返回ACK
            for (int i = receiver_win.base; i < receiver_win.base + RECV_WINDOW_SIZE &&
                 i < TOTAL_PACKETS; ++i) {
                if (!receiver_win.received[i] && !should_drop()) {
                    receiver_win.received[i] = 1;
                    std::cout << "[Receiver] Received packet " << i << "\n";
                }
            }
            
            // 清空已接收的连续包
            while (receiver_win.base < TOTAL_PACKETS &&
                   receiver_win.received[receiver_win.base]) {
                std::cout << "[Receiver] Sending ACK for packet " << receiver_win.base << "\n";
                sender_win.ack_received[receiver_win.base] = 1;
                receiver_win.base++;
                receiver_win.received_count++;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

static void monitor_thread_func() {
    while (!done) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        {
            std::unique_lock<std::mutex> lock(mtx);
            
            std::cout << "\n=== Window State (clock=" << (++global_clock) << ") ===\n";
            
            std::cout << "[Sender Window]\n";
            std::cout << "  Base: " << sender_win.base << ", Next: " << sender_win.next_seq << "\n";
            std::cout << "  Sent packets: " << sender_win.sent_count << "\n";
            std::cout << "  Acked packets: " << sender_win.acked_count << "\n";
            
            // 发送方窗口状态
            int already_acked = sender_win.base;
            int sent_no_ack = sender_win.next_seq - sender_win.base;
            int can_send_not_sent = (sender_win.base + SEND_WINDOW_SIZE) - sender_win.next_seq;
            int cannot_send = TOTAL_PACKETS - (sender_win.base + SEND_WINDOW_SIZE);
            
            std::cout << "  Already acked and delivered: " << already_acked << " packets\n";
            std::cout << "  Sent but not acked: " << sent_no_ack << " packets\n";
            std::cout << "  Can send but not sent: " << (can_send_not_sent > 0 ? can_send_not_sent : 0) << " packets\n";
            std::cout << "  Cannot send: " << (cannot_send > 0 ? cannot_send : 0) << " packets\n";
            
            std::cout << "[Receiver Window]\n";
            std::cout << "  Base: " << receiver_win.base << "\n";
            std::cout << "  Received packets: " << receiver_win.received_count << "\n";
            
            int delivered = receiver_win.base;
            int can_recv = RECV_WINDOW_SIZE;
            int cannot_recv = TOTAL_PACKETS - (receiver_win.base + RECV_WINDOW_SIZE);
            
            std::cout << "  Already delivered to host: " << delivered << " packets\n";
            std::cout << "  Can receive: " << can_recv << " packets\n";
            std::cout << "  Cannot receive: " << (cannot_recv > 0 ? cannot_recv : 0) << " packets\n";
        }
    }
}

int main() {
    srand((unsigned int)time(NULL));
    
    std::cout << "===== 滑动窗口协议模拟 =====\n";
    std::cout << "发送窗口大小: " << SEND_WINDOW_SIZE << "\n";
    std::cout << "接收窗口大小: " << RECV_WINDOW_SIZE << "\n";
    std::cout << "总包数: " << TOTAL_PACKETS << "\n";
    std::cout << "丢包率: " << (int)(LOSS_RATE * 100) << "%\n\n";
    
    init_windows();
    
    // 创建线程
    std::thread sender(sender_thread_func);
    std::thread receiver(receiver_thread_func);
    std::thread monitor(monitor_thread_func);
    
    // 等待发送和接收完成
    sender.join();
    receiver.join();
    done = true;
    
    // 最后输出一次状态
    std::cout << "\n=== Final Window State ===\n";
    std::cout << "Sender: base=" << sender_win.base << ", next=" << sender_win.next_seq << "\n";
    std::cout << "Receiver: base=" << receiver_win.base << "\n";
    std::cout << "Total sent: " << sender_win.sent_count << ", acked: " << sender_win.acked_count << "\n";
    
    monitor.join();
    cleanup_windows();
    
    std::cout << "\n===== 模拟完成 =====\n";
    return 0;
}
