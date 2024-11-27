import socket
import struct
import numpy as np
import time
import os

HOST = '127.0.0.1'
PORT = 5050

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    client.connect((HOST, PORT))
    print("successfully connected")
except socket.error:
    print("couldn't connect", socket.error)

start = time.time()

# Create COM directory if not exists
os.makedirs("result/com", exist_ok=True)

files_to_send = [
    "result/final/26_out.txt",
    "result/final/26_out.txt"
]

# for all actors
for actor_idx, file_path in enumerate(files_to_send):
    print(f"Processing Actor {actor_idx}...")

    # 1. Receive Array from Unreal
    array_size_bytes = client.recv(4)
    array_size = struct.unpack('!I', array_size_bytes)[0]
    array_data_bytes = client.recv(array_size * 4)
    array_data = np.frombuffer(array_data_bytes, dtype=np.float32)
    print(f"Received Array for Actor {actor_idx}: {array_data}")

    # Write vertices (COM data)
    filename = f"result/com/{actor_idx}_in.txt"
    with open(filename, 'w') as obj_file:
        num_vertices = len(array_data) // 3
        for i in range(num_vertices):
            obj_file.write(f"{array_data[i*3]} {array_data[i*3+1]} {array_data[i*3+2]}\n")
        print(f"Written {filename}")

    # 2. Send Array to Unreal
    with open(file_path, "r") as file:
        data = file.read().strip()
    clusteredIdx = list(map(int, data.strip('[]').split(',')))

    # Convert to array and include size
    array = np.array(clusteredIdx, dtype=np.int32)
    array = np.insert(array, 0, len(array))
    array_bytes = array.tobytes()

    client.sendall(array_bytes)
    print(f"Sent cluster array for Actor {actor_idx} from {file_path}")

end = time.time()
print(f"total time: {end-start:.2f} seconds")

client.close()