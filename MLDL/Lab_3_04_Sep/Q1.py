"""
Q1. Linear SVM — Find a Separating Hyperplane
Dataset
X
1X
2Clas
s
22-1
44-1
40+1
04+1
Tasks
1. Plot the points in 2D.
2. Find one linear separating hyperplane (w_1 x_1 + w_2 x_2 + b = 0).
3. Scale (w,b) so that the functional margin of each support vector equals 1 (canonical form).
4. Write the decision function (f(x) = (w^T x + b)).
Expected Output –
One valid hyperplane: e.g., (x_1 - x_2 = 0) (or equivalent form).
- Support vectors ≈ ?
- Margin width (= ) ≈ ?
- Decision rule: ?
"""

import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import math
from sklearn.svm import SVC

data = pd.read_csv("_q1_dataset.csv")
data = data.dropna()

# Data Cleanning Not required

x1_points = np.array(data.loc[data['Class'] == 1, 'X1'])
y1_points = np.array(data.loc[data['Class'] == 1, 'X2'])
x2_points = np.array(data.loc[data['Class'] == -1, 'X1'])
y2_points = np.array(data.loc[data['Class'] == -1, 'X2'])

plt.plot(x1_points, y1_points, 'o')
plt.plot(x2_points, y2_points, 'o')
# plt.show()
plt.savefig("_q1_plot.png")

x_points = np.array(data.loc[:, 'X1'])
y_points = np.array(data.loc[:, 'X2'])

def calculate_distance(point: np.ndarray, w: np.ndarray, b):
    distance = w.dot(point) + b
    distance = abs(distance) / abs(w.dot(w))
    return distance

def main():
    clf = SVC(kernel='linear')
    # clf.fit(x_points, y_points)
    w = np.array([1, -1]).transpose()
    b = 2

    print(f"Linear Seperating Hyperplane: \n\tw = {w}\n\tb = {b}")

    # print("Distance Between Points and HyperPlane:")

    min_distance1 = math.inf
    support_vector1 = []
    min_distance2 = math.inf
    support_vector2 = []

    for i in range(len(x1_points)):
        point = np.array([x1_points[i], y1_points[i]])
        distance = calculate_distance(point, w, b)
        # print(f"{point}) = {distance}")
        if distance < min_distance1:
            min_distance1 = distance
            support_vector1 = []
            support_vector1.append(point)
        elif distance == min_distance1:
            support_vector1.append(point)

    for i in range(len(x2_points)):
        point = np.array([x2_points[i], y2_points[i]])
        distance = calculate_distance(point, w, b)
        # print(f"{point}) = {distance}")
        if distance < min_distance2:
            min_distance2 = distance
            support_vector2 = []
            support_vector2.append(point)
        elif distance == min_distance2:
            support_vector2.append(point)
        # print(distance, min_distance2)
    
    print(f"Support Vectors for Class 1: {support_vector1}")
    print(f"Support Vectors for Class 2: {support_vector2}")

    margin_width = 2 / w.dot(w)
    print(f"Margin Width: {margin_width}")

if __name__ == "__main__":
    main()