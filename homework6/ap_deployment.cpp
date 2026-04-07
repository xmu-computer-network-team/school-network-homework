// AP部署模拟程序
// 计算教学楼AP最佳位置和信道分配

#include <iostream>
#include <cmath>
#include <vector>

// 教学楼参数
const double BUILDING_WIDTH = 100.0;   // 宽度 100m
const double BUILDING_LENGTH = 80.0;   // 长度 80m
const int BUILDING_FLOORS = 3;         // 3层
const double FLOOR_HEIGHT = 3.5;       // 每层高3.5m

// 墙体衰减参数 (dB)
const double BEARING_WALL_ATTEN = 12.0;   // 承重墙（钢筋混凝土，厚度25cm）
const double PARTITION_WALL_ATTEN = 6.0;  // 普通隔断墙（石膏板/木板，厚度10cm）
const double GLASS_WALL_ATTEN = 7.0;      // 玻璃幕墙（厚度5cm）

// 走廊参数
const double CORRIDOR_WIDTH = 2.0;  // 走廊宽度2m，贯穿每层中心区域

// 教室参数
const double CLASSROOM_WIDTH = 15.0;   // 教室宽度15m
const double CLASSROOM_LENGTH = 10.0;  // 教室长度10m，分布两侧

// AP参数
const double AP_POWER = 20.0;        // 发射功率 dBm
const double AP_FREQ_24G = 2.4;      // 2.4GHz频率
const double AP_FREQ_5G = 5.0;       // 5GHz频率

// 目标信号强度阈值
const double TARGET_SIGNAL = -65.0;  // 目标信号强度 ≥-65dBm（保证稳定连接）

// AP覆盖半径估算
// 2.4GHz: 15-25m（无墙体）/ 8-15m（穿透1堵承重墙）
const double COVERAGE_24G_NO_WALL = 20.0;      // 2.4GHz无墙体覆盖半径
const double COVERAGE_24G_ONE_WALL = 11.0;     // 2.4GHz穿透1堵承重墙覆盖半径
// 5GHz: 10-15m（无墙体）/ 5-8m（穿透1堵承重墙）
const double COVERAGE_5G_NO_WALL = 12.0;       // 5GHz无墙体覆盖半径
const double COVERAGE_5G_ONE_WALL = 6.0;       // 5GHz穿透1堵承重墙覆盖半径

// AP结构体
struct AP {
    double x, y, z;   // 坐标
    int channel;       // 信道
    int freq_band;     // 频段 (0: 2.4GHz, 1: 5GHz)
    int floor;         // 楼层
};

// 计算自由空间路径损耗
double path_loss(double distance, double freq_ghz) {
    if (distance < 1.0) distance = 1.0;
    // FSPL公式: 20*log10(d) + 20*log10(f) + 32.44
    return 20.0 * log10(distance) + 20.0 * log10(freq_ghz * 1000.0) + 32.44;
}

// 计算墙体衰减
double wall_attenuation(double x, double y, int floor) {
    double atten = 0.0;
    
    // 检查是否穿过走廊墙 (走廊在中心，y=40m处)
    if (y > (BUILDING_LENGTH/2 - CORRIDOR_WIDTH/2) && 
        y < (BUILDING_LENGTH/2 + CORRIDOR_WIDTH/2)) {
        // 在走廊内，不增加衰减
    } else {
        // 穿过走廊墙
        atten += PARTITION_WALL_ATTEN;
    }
    
    // 检查教室隔断墙 (每15m一道)
    int num_walls = (int)(x / CLASSROOM_WIDTH);
    atten += num_walls * PARTITION_WALL_ATTEN * 0.3;  // 估算穿墙概率
    
    return atten;
}

// 计算信号强度
double signal_strength(const AP& ap, double x, double y, int floor) {
    // 计算距离
    double dx = x - ap.x;
    double dy = y - ap.y;
    double dz = (floor - ap.floor) * FLOOR_HEIGHT;
    double distance = sqrt(dx*dx + dy*dy + dz*dz);
    
    // 计算路径损耗
    double freq = (ap.freq_band == 0) ? AP_FREQ_24G : AP_FREQ_5G;
    double loss = path_loss(distance, freq);
    
    // 加上墙体衰减
    loss += wall_attenuation(x, y, floor);
    
    // 楼层间衰减
    if (floor != ap.floor) {
        loss += abs(floor - ap.floor) * 15.0;  // 每层楼板衰减15dB
    }
    
    return AP_POWER - loss;
}

// 计算AP最佳位置
void calculate_ap_positions(std::vector<AP>& aps) {
    // 每层部署4个AP
    // 位置：教室中心点附近
    
    for (int floor = 1; floor <= BUILDING_FLOORS; ++floor) {
        double z = (floor - 1) * FLOOR_HEIGHT + 1.5;  // AP安装高度
        
        // 左侧教室AP
        AP ap1 = {25.0, 20.0, z, 1, 0, floor};   // 2.4GHz信道1
        AP ap2 = {75.0, 20.0, z, 6, 0, floor};   // 2.4GHz信道6
        AP ap3 = {25.0, 60.0, z, 11, 0, floor};  // 2.4GHz信道11
        AP ap4 = {75.0, 60.0, z, 36, 1, floor};  // 5GHz信道36
        
        aps.push_back(ap1);
        aps.push_back(ap2);
        aps.push_back(ap3);
        aps.push_back(ap4);
    }
}

// 生成信号热力图数据
void generate_heatmap(const std::vector<AP>& aps, int floor) {
    std::cout << "\n=== 第" << floor << "层信号热力图 ===" << std::endl;
    std::cout << "坐标范围: X[0-100m], Y[0-80m]" << std::endl;
    std::cout << "信号强度图例: * (> -50dBm), + (> -60dBm), - (> -70dBm), . (> -80dBm), 空格 (< -80dBm)" << std::endl;
    
    // 简化的热力图，10m分辨率
    for (double y = 0; y <= BUILDING_LENGTH; y += 8.0) {
        for (double x = 0; x <= BUILDING_WIDTH; x += 10.0) {
            // 找最强信号
            double max_signal = -100.0;
            for (size_t i = 0; i < aps.size(); ++i) {
                if (aps[i].floor == floor) {
                    double sig = signal_strength(aps[i], x, y, floor);
                    if (sig > max_signal) max_signal = sig;
                }
            }
            
            // 显示信号强度
            if (max_signal > -50.0) std::cout << "*";
            else if (max_signal > -60.0) std::cout << "+";
            else if (max_signal > -70.0) std::cout << "-";
            else if (max_signal > -80.0) std::cout << ".";
            else std::cout << " ";
        }
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "===== AP部署模拟程序 =====" << std::endl;
    std::cout << "教学楼尺寸: " << BUILDING_WIDTH << "m x " << BUILDING_LENGTH 
              << "m x " << BUILDING_FLOORS << "层" << std::endl;
    
    std::vector<AP> aps;
    
    // 计算AP位置
    calculate_ap_positions(aps);
    
    // 输出AP信息
    std::cout << "\n=== AP部署方案 ===" << std::endl;
    std::cout << "AP编号\t楼层\tX坐标\tY坐标\t信道\t频段" << std::endl;
    for (size_t i = 0; i < aps.size(); ++i) {
        std::cout << i+1 << "\t" << aps[i].floor << "\t" 
                  << aps[i].x << "m\t" << aps[i].y << "m\t"
                  << aps[i].channel << "\t" 
                  << (aps[i].freq_band == 0 ? "2.4GHz" : "5GHz") << std::endl;
    }
    
    // 信道分配说明
    std::cout << "\n=== 信道分配策略 ===" << std::endl;
    std::cout << "2.4GHz频段: 使用信道1、6、11避免同频干扰" << std::endl;
    std::cout << "5GHz频段: 使用信道36、40、44、48等非重叠信道" << std::endl;
    
    // 生成每层热力图
    for (int floor = 1; floor <= BUILDING_FLOORS; ++floor) {
        generate_heatmap(aps, floor);
    }
    
    return 0;
}
