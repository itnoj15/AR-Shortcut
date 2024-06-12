import serial
import socket

# 유니티 설정
UNITY_IP = "172.20.68.87"  # 유니티가 실행되는 PC의 IP 주소
UNITY_PORT = 9999

# 파이썬 UDP 서버 설정
SERVER_IP = "0.0.0.0"
SERVER_PORT = 8888

# 소켓 생성
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((SERVER_IP, SERVER_PORT))

SensorManager = serial.Serial('/dev/cu.usbmodem1201', 115200)

print(f"Python UDP server listening on {SERVER_IP}:{SERVER_PORT}")

while True:
    # 시리얼 입력을 읽고 디코딩
    gesture = SensorManager.readline().decode().strip()
    response = f"Processed: {gesture}".encode()

    # 유니티로 데이터 전송
    sock.sendto(response, (UNITY_IP, UNITY_PORT))
    print(f"Sent message to Unity: {response.decode()}")
