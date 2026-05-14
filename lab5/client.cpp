// LicenseClient.cpp - 修复版（正确处理服务器重启）

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <string>
#include <thread>
#include <windows.h>

using namespace std;

#define SERVER_PORT 2020
#define HEARTBEAT_INTERVAL 8

SOCKET g_sock;
sockaddr_in g_serverAddr;
int g_pid = 0;
string g_ticket = "";
bool g_hasTicket = false;
bool g_running = true;

void Log(const string& msg) {
    cout << "[CLIENT] " << msg << endl;
}

string SendRequest(const string& request) {
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    
    sendto(g_sock, request.c_str(), request.length(), 0, (sockaddr*)&g_serverAddr, sizeof(g_serverAddr));
    
    // 设置接收超时 2 秒
    int timeout = 2000;
    setsockopt(g_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    int bytes = recvfrom(g_sock, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
    
    // 恢复为阻塞模式
    timeout = 0;
    setsockopt(g_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    if (bytes <= 0) {
        return "";
    }
    
    return string(buffer);
}

bool GetTicket() {
    string request = "HELO " + to_string(g_pid);
    string response = SendRequest(request);
    
    Log("Request: " + request);
    
    if (response.empty()) {
        Log("No response (server down?), retrying...");
        return false;
    }
    
    Log("Response: " + response);
    
    if (response.substr(0, 4) == "TICK") {
        g_ticket = response.substr(5);
        g_hasTicket = true;
        Log("Ticket acquired: " + g_ticket);
        return true;
    } else {
        Log("Ticket denied: " + response);
        return false;
    }
}

void ReleaseTicket() {
    if (!g_hasTicket) return;
    string request = "GBYE " + g_ticket;
    string response = SendRequest(request);
    Log("Release: " + request);
    Log("Response: " + response);
    g_hasTicket = false;
}

bool ValidateTicket() {
    if (!g_hasTicket) return false;
    string request = "VALID " + g_ticket;
    string response = SendRequest(request);
    
    if (response.empty()) {
        Log("Heartbeat: no response");
        return false;
    }
    
    Log("Heartbeat: " + response);
    return response.substr(0, 4) == "GOOD";
}

void HeartbeatThreadFunc() {
    while (g_running) {
        Sleep(HEARTBEAT_INTERVAL * 1000);
        
        if (g_hasTicket) {
            if (!ValidateTicket()) {
                Log("Ticket invalid or server down, reacquiring...");
                g_hasTicket = false;
                
                // 重试获取票据
                while (!GetTicket()) {
                    Log("Retry in 3 seconds...");
                    Sleep(3000);
                }
            }
        }
    }
}

int main() {
    cout << "=== Licensed Program ===" << endl;
    
    g_pid = GetCurrentProcessId();
    Log("My PID: " + to_string(g_pid));
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Log("WSAStartup failed");
        return 1;
    }
    
    g_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sock == INVALID_SOCKET) {
        Log("Socket creation failed");
        WSACleanup();
        return 1;
    }
    
    memset(&g_serverAddr, 0, sizeof(g_serverAddr));
    g_serverAddr.sin_family = AF_INET;
    g_serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    g_serverAddr.sin_port = htons(SERVER_PORT);
    
    Log("Requesting license...");
    while (!GetTicket()) {
        Log("Retry in 3 seconds...");
        Sleep(3000);
    }
    
    // 启动心跳线程
    thread heartbeatThread(HeartbeatThreadFunc);
    heartbeatThread.detach();
    
    cout << "\n=== PROGRAM RUNNING ===" << endl;
    cout << "Ticket: " << g_ticket << endl;
    cout << "Press 'q' to quit" << endl;
    
    int seconds = 0;
    while (g_running) {
        if (GetAsyncKeyState('Q') & 0x8000) {
            g_running = false;
            break;
        }
        
        seconds++;
        if (seconds % 5 == 0) {
            cout << "[Running] " << seconds << " seconds" << endl;
        }
        
        Sleep(1000);
    }
    
    ReleaseTicket();
    
    closesocket(g_sock);
    WSACleanup();
    
    Log("Program exited");
    return 0;
}