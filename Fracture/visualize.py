import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import re

def read_vertices(file_path):
    vertices = []
    with open(file_path, 'r') as file:
        for line in file:
            parts = line.split()
            x, y, z = map(float, parts[:3])
            vertices.append((x, y, z))
    return vertices

def read_vertices_log(file_path):
    vertices = []
    pattern = re.compile(r'LogTemp: Warning: ([-\d.]+) ([-\d.]+) ([-\d.]+)')
    with open(file_path, 'r') as file:
        for line in file:
            match = pattern.search(line)
            if match:
                x, y, z = map(float, match.groups())
                vertices.append((x, y, z))
    return vertices

def plot_multiple_files(file_color_pairs):
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    for file_path, color in file_color_pairs:
        vertices = read_vertices(file_path)

        xs = [vertex[0] for vertex in vertices]
        ys = [vertex[1] for vertex in vertices]
        zs = [vertex[2] for vertex in vertices]

        ax.scatter(xs, ys, zs, c=color, marker='o', label=file_path)

    ax.set_xlabel('X Label')
    ax.set_ylabel('Y Label')
    ax.set_zlabel('Z Label')
    ax.legend()

    plt.show()

file_color_pairs = [
    ('vertices.txt', 'g')
]

plot_multiple_files(file_color_pairs)
