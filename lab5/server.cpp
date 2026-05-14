// LicenseServer.cpp - 完整版（自动回收失效票据）

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <string>
#include <map>
#include <ctime>
#include <vector>
#include <thread>
#include <windows.h>

using namespace std;

#define SERVER_PORT 2020
#define MAX_USERS 3
#define CHECK_INTERVAL 10  // 每10秒检查一次

SOCKET g_sock;
sockaddr_in g_addr;
map<int, int> g_tickets;  // ticketId -> pid
int g_ticketCount = 0;
CRITICAL_SECTION g_cs;
bool g_running = true;

void Log(const string& msg) {
    time_t now = time(NULL);
    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);
    string s = timeStr;
    s.pop_back();
    cout << "[SERVER][" << s << "] " << msg << endl;
}

// 检查进程是否存活
bool IsProcessAlive(int pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL) return false;
    DWORD exitCode;
    GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);
    return (exitCode == STILL_ACTIVE);
}

// 回收失效票据（核心功能）
void ReclaimTickets() {
    EnterCriticalSection(&g_cs);
    
    vector<int> toRemove;
    for (auto& pair : g_tickets) {
        int ticketId = pair.first;
        int pid = pair.second;
        
        if (!IsProcessAlive(pid)) {
            toRemove.push_back(ticketId);
            Log("Reclaimed ticket " + to_string(ticketId) + " (PID " + to_string(pid) + " died)");
        }
    }
    
    for (int id : toRemove) {
        g_tickets.erase(id);
        g_ticketCount--;
    }
    
    if (toRemove.size() > 0) {
        Log("Active users: " + to_string(g_ticketCount) + "/" + to_string(MAX_USERS));
    }
    
    LeaveCriticalSection(&g_cs);
}

// 定时回收线程
void ReclaimThreadFunc() {
    while (g_running) {
        Sleep(CHECK_INTERVAL * 1000);
        ReclaimTickets();
    }
}

int main() {
    cout << "=== License Server ===" << endl;
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed" << endl;
        return 1;
    }
    
    g_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sock == INVALID_SOCKET) {
        cout << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }
    
    memset(&g_addr, 0, sizeof(g_addr));
    g_addr.sin_family = AF_INET;
    g_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    g_addr.sin_port = htons(SERVER_PORT);
    
    if (bind(g_sock, (sockaddr*)&g_addr, sizeof(g_addr)) == SOCKET_ERROR) {
        cout << "Bind failed" << endl;
        closesocket(g_sock);
        WSACleanup();
        return 1;
    }
    
    InitializeCriticalSection(&g_cs);
    
    // 启动回收线程
    thread reclaimThread(ReclaimThreadFunc);
    reclaimThread.detach();
    
    Log("Server started on port " + to_string(SERVER_PORT));
    Log("Max users: " + to_string(MAX_USERS));
    
    char buffer[512];
    sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);
    
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recvfrom(g_sock, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&clientAddr, &clientLen);
        if (bytes <= 0) continue;
        
        string request(buffer);
        Log("Received: " + request);
        
        string response;
        
        // HELO 请求
        if (request.substr(0, 4) == "HELO") {
            int pid = atoi(request.substr(5).c_str());
            
            EnterCriticalSection(&g_cs);
            
            if (g_ticketCount >= MAX_USERS) {
                response = "FAIL no tickets";
                Log("DENIED PID " + to_string(pid));
            } else {
                int ticketId = 1;
                while (g_tickets.find(ticketId) != g_tickets.end()) ticketId++;
                g_tickets[ticketId] = pid;
                g_ticketCount++;
                response = "TICK " + to_string(pid) + "." + to_string(ticketId);
                Log("GRANTED ticket " + to_string(ticketId) + " to PID " + to_string(pid));
            }
            
            LeaveCriticalSection(&g_cs);
        }
        // GBYE 请求
        else if (request.substr(0, 4) == "GBYE") {
            string ticketStr = request.substr(5);
            size_t dotPos = ticketStr.find('.');
            if (dotPos != string::npos) {
                int ticketId = atoi(ticketStr.substr(dotPos + 1).c_str());
                
                EnterCriticalSection(&g_cs);
                if (g_tickets.find(ticketId) != g_tickets.end()) {
                    g_tickets.erase(ticketId);
                    g_ticketCount--;
                    response = "THNX bye";
                    Log("RELEASED ticket " + to_string(ticketId));
                } else {
                    response = "FAIL invalid";
                }
                LeaveCriticalSection(&g_cs);
            } else {
                response = "FAIL invalid";
            }
        }
        // VALID 请求（用于客户端验证和心跳）
        else if (request.substr(0, 5) == "VALID") {
            string ticketStr = request.substr(6);
            size_t dotPos = ticketStr.find('.');
            if (dotPos != string::npos) {
                int pid = atoi(ticketStr.substr(0, dotPos).c_str());
                int ticketId = atoi(ticketStr.substr(dotPos + 1).c_str());
                
                EnterCriticalSection(&g_cs);
                auto it = g_tickets.find(ticketId);
                if (it != g_tickets.end() && it->second == pid) {
                    response = "GOOD valid";
                } else {
                    response = "FAIL invalid";
                }
                LeaveCriticalSection(&g_cs);
            } else {
                response = "FAIL invalid";
            }
        }
        else {
            response = "FAIL unknown";
        }
        
        sendto(g_sock, response.c_str(), response.length(), 0, (sockaddr*)&clientAddr, clientLen);
        Log("Responded: " + response);
    }
    
    DeleteCriticalSection(&g_cs);
    closesocket(g_sock);
    WSACleanup();
    return 0;
}