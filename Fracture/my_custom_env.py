import socket
import struct
import numpy as np

HOST = '127.0.0.1'
PORT = 5050

class my_custom_env:
    def __init__(self, id):
        self.id = id
        self.hasConnection = False
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            self.client.connect((HOST, PORT))
            print(f"[ENV {self.id}] Successfully connected to Unreal")
            self.hasConnection = True
        except socket.error as e:
            print(f"[ENV {self.id}] Couldn't connect: {e}")

    def receive_array(self):
        """Receives an array from Unreal."""
        try:
            array_size_bytes = self.client.recv(4)
            array_size = struct.unpack('!I', array_size_bytes)[0]
            array_data_bytes = self.client.recv(array_size * 4)
            array_data = np.frombuffer(array_data_bytes, dtype=np.float32)
            print(f"[ENV {self.id}] Received Array: {array_data}")
            return array_data
        except Exception as e:
            print(f"[ENV {self.id}] Error receiving array: {e}")
            return None

    def send_array(self, file_path):
        """Sends an array to Unreal."""
        try:
            with open(file_path, "r") as file:
                data = file.read().strip()
            clustered_idx = list(map(int, data.strip('[]').split(',')))

            array = np.array(clustered_idx, dtype=np.int32)
            array = np.insert(array, 0, len(array))
            array_bytes = array.tobytes()

            self.client.sendall(array_bytes)
            print(f"[ENV {self.id}] Sent Array from file: {file_path}")
        except Exception as e:
            print(f"[ENV {self.id}] Error sending array: {e}")

    def process(self, file_path):
        """Processes send and receive operations for this environment."""
        self.receive_array()  # Receive response
        self.send_array(file_path)  # Send data


# class custom_env():
#     def __init__(self, id):
#         self.id = id
#         self.hasConnection = False
#         self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#
#         try:
#             self.client.connect((HOST, PORT))
#             print(f"[ENV {self.id}] Successfully connected to Unreal")
#             self.hasConnection = True
#         except socket.error as e:
#             print(f"[ENV {self.id}] Couldn't connect: {e}")
#
#     def receive_array(self):
#         """Receives an array from Unreal."""
#         try:
#             # Receive the size of the array
#             array_size_bytes = self.client.recv(4)
#             array_size = struct.unpack('!I', array_size_bytes)[0]
#
#             # Receive the array data
#             array_data_bytes = self.client.recv(array_size * 4)
#             array_data = np.frombuffer(array_data_bytes, dtype=np.float32)
#             print(f"[ENV {self.id}] Received Array: {array_data}")
#
#             return array_data
#         except Exception as e:
#             print(f"[ENV {self.id}] Error receiving array: {e}")
#             return None
#
#     def send_array(self, file_path):
#         """Sends an array to Unreal."""
#         try:
#             # Read array data from file
#             with open(file_path, "r") as file:
#                 data = file.read().strip()
#             clusteredIdx = list(map(int, data.strip('[]').split(',')))
#
#             # Convert to bytes and prepend the size of the array
#             array = np.array(clusteredIdx, dtype=np.int32)
#             array = np.insert(array, 0, len(array))
#             array_bytes = array.tobytes()
#
#             # Send the array
#             self.client.sendall(array_bytes)
#             print(f"[ENV {self.id}] Sent Array from file: {file_path}")
#         except Exception as e:
#             print(f"[ENV {self.id}] Error sending array: {e}")
#
#     def step(self, client, action):
#
#         # array to send
#         array = np.array([self.id, action], dtype=np.float32)
#         # add array size
#         # array = np.insert(array, 0, len(array))
#         # send array size and content
#         array_bytes = array.tobytes()
#         self.client.sendall(array_bytes)
#
#         state = []
#
#         # wait till you get a response / waiting for new state
#         while (True):
#             array_size_bytes = self.client.recv(4)
#             if not array_size_bytes:
#                 break
#             array_size = struct.unpack('!I', array_size_bytes)[0]  # 4byte integer data
#             # receive array data
#             array_data_bytes = self.client.recv(array_size * 4)
#             array_data = np.frombuffer(array_data_bytes, dtype=np.float32)
#             # print("Step Array:", array_data)
#             state = array_data.copy()
#             break
#
#         reward = 1
#
#         done = False
#         if (abs(state[0]) > 30):
#             done = True
#         elif (abs(state[1] > 800)):
#             done = True
#
#         return state, reward, done
#
#     def reset(self, client):
#
#         array = np.array([self.id, 10], dtype=np.float32)
#         # array = np.insert(array, 0, len(array))
#         array_bytes = array.tobytes()
#         self.client.sendall(array_bytes)
#
#         state = [1, 1, 1, 1]
#
#         # wait till you get a response
#         while (True):
#             array_size_bytes = self.client.recv(4)
#             if not array_size_bytes:
#                 break
#             array_size = struct.unpack('!I', array_size_bytes)[0]
#             # receive array data
#             array_data_bytes = self.client.recv(array_size * 4)
#             array_data = np.frombuffer(array_data_bytes, dtype=np.float32)
#             # print("Reset Array:", array_data)
#             break
#
#         state = [0, 0, 0, 0]
#         return state
#
# env = custom_env()
#
# for i in range(2):
#     done = False
#     state = env.reset()
#     while not done:
#         n_state, reward, done = env.step()