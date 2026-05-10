#!/usr/bin/env python3
# NTP（网络时间协议）服务器实现
# 用法：python ntp_server.py "2019-05-01 10:41:00"

import socket
import struct
import sys
from datetime import datetime

# NTP 常数
NTP_PORT = 123
NTP_VERSION = 3
NTP_LEAP_INDICATOR = 0  # 无闰秒
NTP_MODE_SERVER = 4
NTP_MODE_CLIENT = 3

# NTP 时间戳起始点（1900-01-01）
NTP_EPOCH = datetime(1900, 1, 1)
UNIX_EPOCH = datetime(1970, 1, 1)
NTP_DELTA = (UNIX_EPOCH - NTP_EPOCH).total_seconds()

def parse_ntp_request(data):
    """解析 NTP 请求包"""
    if len(data) < 48:
        return None
    
    header = struct.unpack("!BBBBBBBBBBBBBBBBIIIIIIIIIIIIIIIIII", data[:48])
    return header

def build_ntp_reply(tx_time, server_time):
    """构建 NTP 回复包"""
    # NTP timestamp = seconds since 1900-01-01
    sec = int(server_time)
    usec = int((server_time - sec) * 0x100000000)
    
    # 构造 NTP 报文
    # LI (2 bits) | VN (3 bits) | Mode (3 bits)
    li_vn_mode = (NTP_LEAP_INDICATOR << 6) | (NTP_VERSION << 3) | NTP_MODE_SERVER
    stratum = 2  # 次级参考源
    poll = 4  # 建议轮询间隔
    precision = 0xFA  # 时钟精度
    
    root_delay = 0x10000  # 到参考源的往返延迟
    root_dispersion = 0x10000  # 根域分散
    ref_id = struct.unpack("!I", b"LOOC")[0]  # 参考时钟标识（"LOOC" = Local Clock）
    ref_ts = struct.pack("!II", sec, usec)  # 参考时间戳
    orig_ts = struct.pack("!II", int(tx_time), int((tx_time - int(tx_time)) * 0x100000000))
    recv_ts = struct.pack("!II", sec, usec)  # 接收时间戳
    trans_ts = struct.pack("!II", sec, usec)  # 发送时间戳
    
    packet = struct.pack("!BBBBBB", li_vn_mode, stratum, poll, precision)
    packet += struct.pack("!HI", root_delay, root_dispersion)
    packet += struct.pack("!I", ref_id)
    packet += ref_ts + orig_ts + recv_ts + trans_ts
    
    return packet

def main():
    if len(sys.argv) < 2:
        print("usage: ntp_server.py \"YYYY-MM-DD HH:MM:SS\"")
        return
    
    time_str = sys.argv[1]
    
    try:
        server_dt = datetime.strptime(time_str, "%Y-%m-%d %H:%M:%S")
    except ValueError:
        print(f"Invalid time format: {time_str}")
        print("Expected format: YYYY-MM-DD HH:MM:SS")
        return
    
    # 转换为 NTP 时间戳
    unix_ts = (server_dt - UNIX_EPOCH).total_seconds()
    ntp_ts = unix_ts + NTP_DELTA
    
    # 创建 UDP 套接字
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        sock.bind(("0.0.0.0", NTP_PORT))
        print(f"NTP server started on port {NTP_PORT}")
        print(f"Server time: {server_dt}")
        
        while True:
            data, addr = sock.recvfrom(1024)
            if len(data) >= 48:
                # 解析请求包中的发送时间戳
                try:
                    tx_ts_bytes = data[40:44]
                    tx_ts = struct.unpack("!I", tx_ts_bytes)[0]
                except:
                    tx_ts = 0
                
                # 构造回复
                reply = build_ntp_reply(tx_ts, ntp_ts)
                sock.sendto(reply, addr)
                print(f"NTP request from {addr} - sent reply")
    
    except OSError as e:
        if e.errno == 13 or "Permission denied" in str(e):
            print(f"Error: need root/admin privilege to bind port {NTP_PORT}")
            print("Try running with: sudo python3 ntp_server.py \"2019-05-01 10:41:00\"")
        else:
            print(f"Error: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
