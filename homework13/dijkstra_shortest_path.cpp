#include <iostream>
#include <vector>
#include <map>
#include <string>

struct Edge {
    int to;
    int w;
};

static const int INF = 1000000000;

int main() {
    int m = 0;
    std::cin >> m;

    // 输入 m 条边，每条边是三元组 (r1 r2 weight)
    std::vector<std::string> names;
    std::map<std::string, int> id;
    std::vector<std::vector<Edge> > g;

    auto get_id = [&](const std::string& s) -> int {
        std::map<std::string, int>::iterator it = id.find(s);
        if (it != id.end()) return it->second;
        int nid = (int)names.size();
        names.push_back(s);
        id[s] = nid;
        g.push_back(std::vector<Edge>());
        return nid;
    };

    for (int i = 0; i < m; ++i) {
        std::string a, b;
        int w;
        std::cin >> a >> b >> w;
        int u = get_id(a);
        int v = get_id(b);
        g[u].push_back((Edge){v, w});
        g[v].push_back((Edge){u, w});
    }

    std::string src_name, dst_name;
    std::cin >> src_name >> dst_name;

    if (id.find(src_name) == id.end() || id.find(dst_name) == id.end()) {
        std::cout << "no path" << std::endl;
        return 0;
    }

    int n = (int)names.size();
    int src = id[src_name];
    int dst = id[dst_name];

    std::vector<int> dist(n, INF);
    std::vector<int> prev(n, -1);
    std::vector<int> vis(n, 0);

    dist[src] = 0;

    // 朴素 Dijkstra，适合实验作业规模
    for (int step = 0; step < n; ++step) {
        int u = -1;
        for (int i = 0; i < n; ++i) {
            if (!vis[i] && (u == -1 || dist[i] < dist[u])) {
                u = i;
            }
        }

        if (u == -1 || dist[u] == INF) break;
        vis[u] = 1;

        for (size_t i = 0; i < g[u].size(); ++i) {
            int v = g[u][i].to;
            int w = g[u][i].w;
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                prev[v] = u;
            }
        }
    }

    if (dist[dst] == INF) {
        std::cout << "no path" << std::endl;
        return 0;
    }

    // 还原路径
    std::vector<int> path;
    for (int x = dst; x != -1; x = prev[x]) {
        path.push_back(x);
    }

    // 输出最短距离和路径
    std::cout << dist[dst] << std::endl;
    for (int i = (int)path.size() - 1; i >= 0; --i) {
        std::cout << names[path[i]];
        if (i) std::cout << " -> ";
    }
    std::cout << std::endl;

    return 0;
}
