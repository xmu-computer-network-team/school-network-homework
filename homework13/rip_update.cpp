#include <iostream>
#include <map>
#include <string>

struct RouteItem {
    std::string next_hop;
    int cost;
};

// RIP 最大代价通常视为16（不可达）
static const int RIP_INFINITY = 16;

static int clamp_cost(int c) {
    if (c < 0) return RIP_INFINITY;
    if (c > RIP_INFINITY) return RIP_INFINITY;
    return c;
}

int main() {
    std::map<std::string, RouteItem> r1_table;
    std::map<std::string, int> r2_table;

    int n1 = 0;
    std::cin >> n1;
    // R1 路由表输入格式:
    // dest next_hop cost
    for (int i = 0; i < n1; ++i) {
        std::string dest;
        RouteItem item;
        std::cin >> dest >> item.next_hop >> item.cost;
        item.cost = clamp_cost(item.cost);
        r1_table[dest] = item;
    }

    int n2 = 0;
    std::cin >> n2;
    // R2 发来的路由表输入格式:
    // dest cost
    for (int i = 0; i < n2; ++i) {
        std::string dest;
        int cost;
        std::cin >> dest >> cost;
        r2_table[dest] = clamp_cost(cost);
    }

    // 假设 R1 到 R2 直连，跳数为 1
    for (std::map<std::string, int>::iterator it = r2_table.begin(); it != r2_table.end(); ++it) {
        std::string dest = it->first;
        int via_r2 = clamp_cost(it->second + 1);

        std::map<std::string, RouteItem>::iterator cur = r1_table.find(dest);
        if (cur == r1_table.end()) {
            RouteItem add;
            add.next_hop = "R2";
            add.cost = via_r2;
            r1_table[dest] = add;
        } else {
            // 若原路由下一跳就是 R2，直接刷新
            if (cur->second.next_hop == "R2") {
                cur->second.cost = via_r2;
            } else {
                // 否则采用更小代价
                if (via_r2 < cur->second.cost) {
                    cur->second.next_hop = "R2";
                    cur->second.cost = via_r2;
                }
            }
        }
    }

    // 输出 R1 新路由表
    // 输出格式: dest next_hop cost
    std::cout << r1_table.size() << std::endl;
    for (std::map<std::string, RouteItem>::iterator it = r1_table.begin(); it != r1_table.end(); ++it) {
        std::cout << it->first << " " << it->second.next_hop << " " << it->second.cost << std::endl;
    }

    return 0;
}
