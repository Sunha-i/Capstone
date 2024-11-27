import socket
import struct
import numpy as np
import time

HOST = '127.0.0.1'
PORT = 5050

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    client.connect((HOST, PORT))
    print("successfully connected")
except socket.error:
    print("couldn't connect", socket.error)

start = time.time()

# Receive Array
array_size_bytes = client.recv(4)
array_size = struct.unpack('!I', array_size_bytes)[0]
array_data_bytes = client.recv(array_size * 4)
array_data = np.frombuffer(array_data_bytes, dtype=np.float32)
print("Received Array: ", array_data)

# Write vertices (COM data)
filename = "hourglass.txt"
with open(filename, 'w') as obj_file:
    num_vertices = len(array_data) // 3
    for i in range(num_vertices):
        obj_file.write(f"{array_data[i*3]} {array_data[i*3+1]} {array_data[i*3+2]}\n")
    print(f"Written {filename}")

# Send Array - sample idx for GC_10_12
filenames = [
    "result/final/21_out.txt"
    "result/final/26_out.txt",
    "result/final/4_out.txt",
]
for idx, filepath in enumerate(filenames):  # filenames 배열의 길이만큼 반복
    with open(filepath, "r") as file:
        data = file.read().strip()
    clusteredIdx = list(map(int, data.strip('[]').split(',')))  # 파일 데이터 파싱
    array = np.array(clusteredIdx, dtype=np.int32)
    array = np.insert(array, 0, len(array))  # 배열 크기 삽입
    array = np.insert(array, 0, idx)  # 배열 크기 삽입
    array_bytes = array.tobytes()  # 바이트 변환
    client.sendall(array_bytes)  # 전송
    print(f"Sent data batch {idx + 1}")

end = time.time()
print(f"total time: {end-start} ms")

client.close()