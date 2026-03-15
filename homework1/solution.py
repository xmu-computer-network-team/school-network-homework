import cv2
import numpy as np
import time
import math

class OpticalCommunication:
    def __init__(self, mode="binary"):
        self.mode = mode
        
        self.binary_palette = {
            1: (0, 0, 0),       # 黑
            0: (255, 255, 255)  # 白
        }
        
        self.octal_palette = {
            0: (0, 0, 0),         # 0 - 黑
            1: (255, 255, 255),   # 1 - 白
            2: (0, 0, 255),       # 2 - 红
            3: (255, 0, 0),       # 3 - 蓝
            4: (0, 255, 0),       # 4 - 绿
            5: (255, 0, 255),     # 5 - 紫 (红+蓝)
            6: (255, 255, 0),     # 6 - 黄 (红+绿)
            7: (0, 255, 255)      # 7 - 青 (蓝+绿)
        }

    def encode(self, msg: int) -> tuple:
        palette = self.binary_palette if self.mode == "binary" else self.octal_palette
        if msg not in palette:
            raise ValueError(f"消息 {msg} 不在调色板范围内")
        return palette[msg]
        

    def decode(self, color: tuple) -> int:
        palette = self.binary_palette if self.mode == "binary" else self.octal_palette
        
        min_distance = float('inf')
        decoded_msg = None
        for msg, palette_color in palette.items():
            distance = math.sqrt(sum((color[i] - palette_color[i]) ** 2 for i in range(3)))
            if distance < min_distance:
                min_distance = distance
                decoded_msg = msg
        return decoded_msg

    def send(self, msg: int):
        target_color = self.encode(msg)
        img = np.full((200, 200, 3), target_color, dtype=np.uint8) # 创建一个纯色图片
        cv2.imshow("Optical Communication", img)
        cv2.waitKey(2000) 
        cv2.destroyAllWindows()
        pass

    def receive(self) -> int:
        cap = cv2.VideoCapture(0)
        time.sleep(1) # 给摄像头一点预热和自动曝光调节的时间
        
        ret, frame = cap.read()
        cap.release()
        
        if not ret:
            raise Exception("无法从摄像头读取画面")
            
        cap = cv2.VideoCapture(0)
        ret, frame = cap.read()
        if ret:
            height, width = frame.shape[:2]
            # 获取中心 100x100 区域
            center_roi = frame[height//2 - 50:height//2 + 50, width//2 - 50:width//2 + 50]
            avg_color = cv2.mean(center_roi)[:3]  
        return self.decode(avg_color)


