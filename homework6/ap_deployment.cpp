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

// 计算两点之间穿过的墙体数量和类型
struct WallCount {
    int bearing_walls;    // 承重墙数量
    int partition_walls;  // 普通隔断墙数量
    int glass_walls;      // 玻璃幕墙数量
};

// 计算从AP到目标点穿过的墙体
WallCount count_walls(double ap_x, double ap_y, double target_x, double target_y) {
    WallCount walls = {0, 0, 0};
    
    // 走廊中心线位置 (y=40m)
    double corridor_center = BUILDING_LENGTH / 2.0;
    double corridor_min = corridor_center - CORRIDOR_WIDTH / 2.0;
    double corridor_max = corridor_center + CORRIDOR_WIDTH / 2.0;
    
    // 判断是否穿过走廊墙（普通隔断墙）
    bool ap_in_corridor = (ap_y >= corridor_min && ap_y <= corridor_max);
    bool target_in_corridor = (target_y >= corridor_min && target_y <= corridor_max);
    
    if (ap_in_corridor != target_in_corridor) {
        walls.partition_walls++;  // 穿过走廊墙
    }
    
    // 计算穿过的教室隔断墙（每15m一道，假设为承重墙）
    int ap_room_x = (int)(ap_x / CLASSROOM_WIDTH);
    int target_room_x = (int)(target_x / CLASSROOM_WIDTH);
    walls.bearing_walls += abs(target_room_x - ap_room_x);
    
    // 计算穿过的横向墙体（教室之间的墙，假设为普通隔断墙）
    int ap_room_y = (int)(ap_y / CLASSROOM_LENGTH);
    int target_room_y = (int)(target_y / CLASSROOM_LENGTH);
    if (ap_room_y != target_room_y && !ap_in_corridor && !target_in_corridor) {
        walls.partition_walls += abs(target_room_y - ap_room_y);
    }
    
    return walls;
}

// 计算墙体衰减
double wall_attenuation(const WallCount& walls) {
    return walls.bearing_walls * BEARING_WALL_ATTEN +
           walls.partition_walls * PARTITION_WALL_ATTEN +
           walls.glass_walls * GLASS_WALL_ATTEN;
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
    
    // 计算墙体衰减
    WallCount walls = count_walls(ap.x, ap.y, x, y);
    loss += wall_attenuation(walls);
    
    // 楼层间衰减（楼板衰减）
    if (floor != ap.floor) {
        loss += abs(floor - ap.floor) * 15.0;  // 每层楼板衰减15dB
    }
    
    return AP_POWER - loss;
}

// 计算AP有效覆盖半径
double get_effective_coverage(int freq_band, int wall_count) {
    if (freq_band == 0) {  // 2.4GHz
        if (wall_count == 0) return COVERAGE_24G_NO_WALL;
        else return COVERAGE_24G_ONE_WALL;
    } else {  // 5GHz
        if (wall_count == 0) return COVERAGE_5G_NO_WALL;
        else return COVERAGE_5G_ONE_WALL;
    }
}

// 计算AP最佳位置（考虑覆盖半径和信号重叠最小化）
void calculate_ap_positions(std::vector<AP>& aps) {
    // 根据覆盖半径计算每层需要的AP数量
    // 教学楼宽度100m，长度80m
    // 2.4GHz覆盖半径约11-20m，5GHz覆盖半径约6-12m
    
    // 每层部署策略：
    // - 左侧教室区域（y: 0-39m）：2个AP
    // - 右侧教室区域（y: 41-80m）：2个AP
    // - 走廊区域（y: 39-41m）：由两侧AP覆盖
    
    for (int floor = 1; floor <= BUILDING_FLOORS; ++floor) {
        double z = (floor - 1) * FLOOR_HEIGHT + 2.5;  // AP安装高度2.5m
        
        // 左侧区域AP（覆盖上半部分教室）
        // 位置选择：教室中心，避免承重墙阻挡
        AP ap1 = {22.5, 15.0, z, 1, 0, floor};    // 2.4GHz信道1，覆盖左侧前部
        AP ap2 = {52.5, 15.0, z, 6, 0, floor};    // 2.4GHz信道6，覆盖中部前部
        AP ap3 = {82.5, 15.0, z, 11, 0, floor};   // 2.4GHz信道11，覆盖右侧前部
        
        // 右侧区域AP（覆盖下半部分教室）
        AP ap4 = {22.5, 65.0, z, 36, 1, floor};   // 5GHz信道36，覆盖左侧后部
        AP ap5 = {52.5, 65.0, z, 40, 1, floor};   // 5GHz信道40，覆盖中部后部
        AP ap6 = {82.5, 65.0, z, 44, 1, floor};   // 5GHz信道44，覆盖右侧后部
        
        aps.push_back(ap1);
        aps.push_back(ap2);
        aps.push_back(ap3);
        aps.push_back(ap4);
        aps.push_back(ap5);
        aps.push_back(ap6);
    }
}

// 计算信号重叠区域
double calculate_overlap(const std::vector<AP>& aps, int floor) {
    int overlap_count = 0;
    int total_points = 0;
    
    for (double x = 0; x <= BUILDING_WIDTH; x += 5.0) {
        for (double y = 0; y <= BUILDING_LENGTH; y += 5.0) {
            int strong_signal_count = 0;
            for (size_t i = 0; i < aps.size(); ++i) {
                if (aps[i].floor == floor) {
                    double sig = signal_strength(aps[i], x, y, floor);
                    if (sig >= TARGET_SIGNAL) {
                        strong_signal_count++;
                    }
                }
            }
            if (strong_signal_count > 1) {
                overlap_count++;
            }
            total_points++;
        }
    }
    
    return (double)overlap_count / total_points * 100.0;
}

// 生成信号热力图数据
void generate_heatmap(const std::vector<AP>& aps, int floor) {
    std::cout << "\n=== 第" << floor << "层信号热力图 ===" << std::endl;
    std::cout << "坐标范围: X[0-100m], Y[0-80m]" << std::endl;
    std::cout << "目标信号强度阈值: " << TARGET_SIGNAL << " dBm" << std::endl;
    std::cout << "信号强度图例:" << std::endl;
    std::cout << "  * : 优秀 (>= -50dBm)" << std::endl;
    std::cout << "  + : 良好 (>= -60dBm)" << std::endl;
    std::cout << "  - : 达标 (>= -65dBm，目标阈值)" << std::endl;
    std::cout << "  . : 较弱 (>= -75dBm)" << std::endl;
    std::cout << "  空格: 信号不足 (< -75dBm)" << std::endl;
    
    // 热力图，5m分辨率
    for (double y = BUILDING_LENGTH; y >= 0; y -= 4.0) {
        for (double x = 0; x <= BUILDING_WIDTH; x += 5.0) {
            // 找最强信号
            double max_signal = -100.0;
            for (size_t i = 0; i < aps.size(); ++i) {
                if (aps[i].floor == floor) {
                    double sig = signal_strength(aps[i], x, y, floor);
                    if (sig > max_signal) max_signal = sig;
                }
            }
            
            // 显示信号强度
            if (max_signal >= -50.0) std::cout << "*";
            else if (max_signal >= -60.0) std::cout << "+";
            else if (max_signal >= TARGET_SIGNAL) std::cout << "-";
            else if (max_signal >= -75.0) std::cout << ".";
            else std::cout << " ";
        }
        std::cout << " | Y=" << (int)y << "m" << std::endl;
    }
    
    // 显示X轴刻度
    std::cout << "  ";
    for (double x = 0; x <= BUILDING_WIDTH; x += 10.0) {
        std::cout << (int)x % 10;
    }
    std::cout << " (X轴: 米)" << std::endl;
}

int main() {
    std::cout << "===== AP部署模拟程序 =====" << std::endl;
    std::cout << "教学楼尺寸: " << BUILDING_WIDTH << "m x " << BUILDING_LENGTH 
              << "m x " << BUILDING_FLOORS << "层" << std::endl;
    std::cout << "每层高度: " << FLOOR_HEIGHT << "m" << std::endl;
    std::cout << "教室尺寸: " << CLASSROOM_WIDTH << "m x " << CLASSROOM_LENGTH << "m" << std::endl;
    std::cout << "走廊宽度: " << CORRIDOR_WIDTH << "m（贯穿每层中心）" << std::endl;
    
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
    
    // 覆盖半径说明
    std::cout << "\n=== AP覆盖半径估算 ===" << std::endl;
    std::cout << "2.4GHz频段:" << std::endl;
    std::cout << "  - 无墙体: " << COVERAGE_24G_NO_WALL << "m" << std::endl;
    std::cout << "  - 穿透1堵承重墙: " << COVERAGE_24G_ONE_WALL << "m" << std::endl;
    std::cout << "5GHz频段:" << std::endl;
    std::cout << "  - 无墙体: " << COVERAGE_5G_NO_WALL << "m" << std::endl;
    std::cout << "  - 穿透1堵承重墙: " << COVERAGE_5G_ONE_WALL << "m" << std::endl;
    
    // 信道分配说明
    std::cout << "\n=== 信道分配策略 ===" << std::endl;
    std::cout << "目标信号强度阈值: " << TARGET_SIGNAL << " dBm（保证稳定连接）" << std::endl;
    std::cout << "2.4GHz频段: 使用信道1、6、11避免同频干扰" << std::endl;
    std::cout << "5GHz频段: 使用信道36、40、44、48等非重叠信道" << std::endl;
    
    // 计算并显示信号重叠区域
    std::cout << "\n=== 信号重叠分析 ===" << std::endl;
    for (int floor = 1; floor <= BUILDING_FLOORS; ++floor) {
        double overlap = calculate_overlap(aps, floor);
        std::cout << "第" << floor << "层信号重叠区域比例: " << overlap << "%" << std::endl;
    }
    
    // 生成每层热力图
    for (int floor = 1; floor <= BUILDING_FLOORS; ++floor) {
        generate_heatmap(aps, floor);
    }
    
    return 0;
}
