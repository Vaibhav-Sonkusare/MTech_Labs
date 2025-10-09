"""
Polynomial Kernel SVM — Find the Feature Mapping
Dataset:
X1
0
1
-1
1
X2
0
1
1
-1
Class
-1
-1
+1
+1
Kernel: Polynomial kernel of degree 2: K(x, z) = (x^T z + 1)^2
Tasks:
1. Write the explicit feature mapping φ(x) corresponding to the kernel.
2. Map all points into the new feature space.
3. Show that the classes are linearly separable in the transformed space.
4. Find one possible separating hyperplane in φ-space.
5. Write the decision function using the kernel trick.
Expected Output:
- Mapping: φ(x) = (x₁², √2 x₁x₂, x₂², √2x₁, √2x₂, 1).
- Hyperplane separating classes in φ-space.
- Decision function expressed via kernel.
"""

import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

data = {
    'X1': [0, 1, -1, 1],
    'X2': [0, 1, 1, -1],
    'Class': [-1, -1, 1, 1]
}
df = pd.DataFrame(data=data)

# part 1
def phi(x):
    x1, x2 = x
    return np.array([
        x1**2,
        np.sqrt(2) * x1 * x2,
        x2**2,
        np.sqrt(2) * x1,
        np.sqrt(2) * x2,
        1
    ])

print("\nTask 1: Feature Mapping φ(x)")
print("φ(x) = (x₁², √2 x₁x₂, x₂², √2x₁, √2x₂, 1)")

# part 2
X = df[['X1', 'X2']].to_numpy()
y = df['Class'].to_numpy().squeeze()
mapped_points = [phi(p) for p in X]

mapped_df = pd.DataFrame(mapped_points, columns=['z₁=x₁²', 'z₂=√2x₁x₂', 'z₃=x₂²', 'z₄=√2x₁', 'z₅=√2x₂', 'z₆=1'])
mapped_df['Class'] = y
print("\nTask 2: Points in new feature space: ")
print(mapped_df.to_string(index=False))

# part 3

print("\nTask 3: Verifying Linear Seperatibility: ")
print("Here z₂ is different for different classes:")
for i, z in enumerate(mapped_points):
    print(f"\tPoint {X[i]} -> z₂ = {z[1]:.4f}, Class = {y[i]}")

# part 4

print("\nTask 4: Find a Seperating Hyperplane")

W = np.array([0, -1, 0, 0, 0, 0])
B = 0.0

print("\tSeperating Hyperplane: -z₂ =  0")
print(f"W = {W}, B = {B}")

for i, z in enumerate(mapped_points):
    decision_value = np.dot(W, z) + B
    predicted = np.sign(decision_value) if decision_value != 0 else -1
    correctness = "Correct" if predicted == y[i] else "Incorrect"
    print(f"Point {X[i]} → Decision = {decision_value:.4f}, Predicted = {predicted}, Actual = {y[i]} → {correctness}")

# part 5

print("\nTask 5: Decision Function Using Kernel Trick ---")
print("Kernel: K(x, z) = (xᵀz + 1)²")
print("Decision function:")
print("f(x) = sign( - √2 * x₁ * x₂ )")
print("or equivalently:")
print("f(x) = sign( -K(x, [1,1]) + K(x, [-1,1]) + K(x, [1,-1]) )")
