"""
Q2. Linear SVM — Identify Support Vectors & Margin
Dataset
X
1X
2Clas
s
15+1
24+1
41-1
52-1
Suppose you found the separating hyperplane (x_1 + x_2 - 6 = 0).
Tasks
1. For each point compute (|w^T x + b| / w) to get its distance to the hyperplane.
2. Identify support vectors (smallest distance among correctly classified points with margin = 1 in canonical
scaling).
3. Compute the margin width (2/w).
Expected Output - Distances: ?
- Support vectors: ?
- Margin width ≈ ?
"""

import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import math

data = pd.read_csv("_q2_dataset.csv")
data = data.dropna()

# Data Cleanning Not required

x1_points = np.array(data.loc[data['Class'] == 1, 'X1'])
y1_points = np.array(data.loc[data['Class'] == 1, 'X2'])
x2_points = np.array(data.loc[data['Class'] == -1, 'X1'])
y2_points = np.array(data.loc[data['Class'] == -1, 'X2'])

plt.plot(x1_points, y1_points, 'o')
plt.plot(x2_points, y2_points, 'o')
# plt.show()
plt.savefig("_q2_plot.png")

x_points = np.array(data.loc[:, 'X1'])
y_points = np.array(data.loc[:, 'X2'])

def calculate_distance(point: np.ndarray, w: np.ndarray, b):
    distance = w.dot(point) + b
    distance = abs(distance) / abs(w.dot(w))
    return distance

def main():
    w = np.array([1, -1]).transpose()
    b = -0.5

    print("Distance Between Points and HyperPlane:")

    min_distance1 = math.inf
    support_vector1 = []
    min_distance2 = math.inf
    support_vector2 = []

    for i in range(len(x1_points)):
        point = np.array([x1_points[i], y1_points[i]])
        distance = calculate_distance(point, w, b)
        print(f"{point}) = {distance}")
        if distance < min_distance1:
            min_distance1 = distance
            support_vector1 = []
            support_vector1.append(point)
        elif distance == min_distance1:
            support_vector1.append(point)

    for i in range(len(x2_points)):
        point = np.array([x2_points[i], y2_points[i]])
        distance = calculate_distance(point, w, b)
        print(f"{point}) = {distance}")
        if distance < min_distance2:
            min_distance2 = distance
            support_vector2 = []
            support_vector2.append(point)
        elif distance == min_distance2:
            support_vector2.append(point)
    
    print(f"Support Vectors for Class 1: {support_vector1}, min distance = {min_distance1}")
    print(f"Support Vectors for Class 2: {support_vector2}, min_distance = {min_distance2}")

    margin_width = 2 / w.dot(w)
    print(f"Margin Width: {margin_width}")

if __name__ == "__main__":
    main()