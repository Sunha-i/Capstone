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

# Send Array - test
array = np.array([1.223, 5.32, 231.23], dtype=np.float32)
array = np.insert(array, 0, len(array))
array_bytes = array.tobytes()
client.sendall(array_bytes)

# Receive Array
start = time.time()
array_size_bytes = client.recv(4)
array_size = struct.unpack('!I', array_size_bytes)[0]
array_data_bytes = client.recv(array_size * 4)
array_data = np.frombuffer(array_data_bytes, dtype=np.float32)
end = time.time()

print("Received Array: ", array_data)
print(f"total time: {end-start} ms")

# Write vertices (COM data)
filename = "vertices.obj"
with open(filename, 'w') as obj_file:
    num_vertices = len(array_data) // 3
    for i in range(num_vertices):
        obj_file.write(f"v {array_data[i*3]} {array_data[i*3+1]} {array_data[i*3+2]}\n")
    print(f"Written {filename}")

client.close()