// CSMA/CD协议模拟程序
// 使用多线程技术模拟载波监听多路访问/冲突检测机制

#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

// 共享介质状态
static bool channel_busy = false;
static pthread_mutex_t channel_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t channel_cond = PTHREAD_COND_INITIALIZER;

// 工作站参数
struct StationParams {
    int id;             // 工作站ID
    int frame_count;    // 要发送的帧数量
};

// 模拟载波监听
static int carrier_sense() {
    pthread_mutex_lock(&channel_mutex);
    bool busy = channel_busy;
    pthread_mutex_unlock(&channel_mutex);
    return busy ? 1 : 0;
}

// 模拟发送数据
static void transmit(int station_id) {
    pthread_mutex_lock(&channel_mutex);
    
    // 检查信道是否空闲
    while (channel_busy) {
        std::cout << "站点 " << station_id << ": 信道忙，等待..." << std::endl;
        pthread_cond_wait(&channel_cond, &channel_mutex);
    }
    
    // 占用信道
    channel_busy = true;
    std::cout << "站点 " << station_id << ": 开始发送数据" << std::endl;
    pthread_mutex_unlock(&channel_mutex);
    
    // 模拟发送时间
    usleep(100000 + rand() % 100000);
    
    // 模拟冲突检测 (10%概率发生冲突)
    if (rand() % 10 == 0) {
        pthread_mutex_lock(&channel_mutex);
        std::cout << "站点 " << station_id << ": 检测到冲突！" << std::endl;
        channel_busy = false;
        pthread_cond_broadcast(&channel_cond);
        pthread_mutex_unlock(&channel_mutex);
        
        // 执行二进制指数退避算法
        int backoff = (rand() % 4) + 1;
        std::cout << "站点 " << station_id << ": 等待 " << backoff << " 个时隙后重试" << std::endl;
        usleep(backoff * 50000);
        return;
    }
    
    // 发送完成
    pthread_mutex_lock(&channel_mutex);
    channel_busy = false;
    std::cout << "站点 " << station_id << ": 发送完成" << std::endl;
    pthread_cond_broadcast(&channel_cond);
    pthread_mutex_unlock(&channel_mutex);
}

// 工作站线程函数
static void* station_thread(void* arg) {
    StationParams* params = (StationParams*)arg;
    
    for (int i = 0; i < params->frame_count; ++i) {
        // 载波监听
        while (carrier_sense()) {
            usleep(10000);
        }
        
        // 尝试发送
        transmit(params->id);
        
        // 帧间间隔
        usleep(50000);
    }
    
    return NULL;
}

// 主函数
int main() {
    srand(time(NULL));
    
    const int NUM_STATIONS = 3;
    pthread_t threads[NUM_STATIONS];
    StationParams params[NUM_STATIONS];
    
    std::cout << "===== CSMA/CD 协议模拟开始 =====" << std::endl;
    
    // 创建工作站线程
    for (int i = 0; i < NUM_STATIONS; ++i) {
        params[i].id = i + 1;
        params[i].frame_count = 2;
        pthread_create(&threads[i], NULL, station_thread, &params[i]);
    }
    
    // 等待所有线程完成
    for (int i = 0; i < NUM_STATIONS; ++i) {
        pthread_join(threads[i], NULL);
    }
    
    std::cout << "===== CSMA/CD 协议模拟结束 =====" << std::endl;
    
    return 0;
}
