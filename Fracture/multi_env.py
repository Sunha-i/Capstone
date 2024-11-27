import socket
import struct
import numpy as np

HOST = '127.0.0.1'
PORT = 5050

file_paths = [
    "result/final/26_out.txt",
    "result/final/26_out.txt"
]

class multi_env:
    def __init__(self, num_env):
        self.envs = []

        for _ in range(num_env):
            env = my_custom_env(_)
            self.envs.append(env)

        self.hasConnection = False
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            self.socket.connect((HOST, PORT))
            print("succeessfully connected")
            self.hasConnection = True
        except socket.error:
            print("couldn't connect", socket.error)

    def reset(self, id):
        state = self.envs[id].reset(self.socket)
        return state

    def process_all(self, file_paths):
        """Processes all environments with given file paths."""
        for id, file_path in enumerate(file_paths):
            print(f"Processing ENV {id} with file: {file_path}")
            self.envs[id].process(file_path)

    def step(self, actions):
        obs = []
        rewards = []
        dones = []

        for i, action in enumerate(actions):
            # ID와 액션을 함께 전송
            array = np.array([i, action], dtype=np.float32)
            self.socket.sendall(array.tobytes())

            # 응답 받기
            state = []
            while True:
                array_size_bytes = self.socket.recv(4)
                if not array_size_bytes:
                    break
                array_size = struct.unpack('!I', array_size_bytes)[0]
                array_data_bytes = self.socket.recv(array_size * 4)
                array_data = np.frombuffer(array_data_bytes, dtype=np.float32)
                state = array_data.copy()
                break

            reward = 1
            done = abs(state[0]) > 30 or abs(state[1]) > 800

            obs.append(state.tolist())
            rewards.append(reward)
            dones.append(done)

            if done:
                self.reset(i)

        return obs, rewards, dones


class my_custom_env:
    def __init__(self, id):
        self.id = id
        self.file_path = file_paths[id]

    def receive_array(self, client):
        """Receives an array from Unreal."""
        try:
            array_size_bytes = client.recv(4)
            array_size = struct.unpack('!I', array_size_bytes)[0]
            array_data_bytes = client.recv(array_size * 4)
            array_data = np.frombuffer(array_data_bytes, dtype=np.float32)
            print(f"[ENV {self.id}] Received Array: {array_data}")
            return array_data
        except Exception as e:
            print(f"[ENV {self.id}] Error receiving array: {e}")
            return None

    def send_array(self, client):
        """Sends an array to Unreal."""
        try:
            with open(self.file_path, "r") as file:
                data = file.read().strip()
            clustered_idx = list(map(int, data.strip('[]').split(',')))

            array = np.array(clustered_idx, dtype=np.int32)
            array = np.insert(array, 0, len(array))
            array = np.insert(array, 0, self.id)
            array_bytes = array.tobytes()

            client.sendall(array_bytes)
            print(f"[ENV {self.id}] Sent Array from file: {self.file_path}")
        except Exception as e:
            print(f"[ENV {self.id}] Error sending array: {e}")

    def reset(self, client):
        """Processes send and receive operations for this environment."""
        self.receive_array(client)
        self.send_array(client)




# class multi_env:
#     def __init__(self, num_env):
#         self.envs = []
#
#         for _ in range(num_env):
#             env = custom_env.custom_env(_)
#             self.envs.append(env)
#
#         self.hasConnection = False
#         self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#
#         try:
#             self.socket.connect((HOST, PORT))
#             print("successfully connected")
#             self.hasConnection = True
#         except socket.error:
#             print("couldn't connect", socket.error)
#
#     def reset(self, id):
#         state = self.envs[id].reset(self.socket)
#         return state
#
#     def step(self, actions):
#         obs = []
#         rewards = []
#         dones = []
#
#         for i in range(len(actions)):
#             env = self.envs[i]
#             ac = actions[i]
#             ob, rew, done = env.step(self.socket, ac)
#             obs.append(ob.tolist())
#             rewards.append(rew)
#             dones.append(done)
#
#             if done:
#                 env.reset(self.socket)
#
#         return obs, rewards, dones