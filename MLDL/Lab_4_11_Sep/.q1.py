import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# --- Dataset ---
data = {
    'X1': [0, 1, -1, 1],
    'X2': [0, 1, 1, -1],
    'Class': [-1, -1, 1, 1]
}
df = pd.DataFrame(data)

# --- Plot the original points to show non-separability ---
class_pos = df[df['Class'] == 1]
class_neg = df[df['Class'] == -1]
plt.figure(figsize=(8, 6))
plt.scatter(class_pos['X1'], class_pos['X2'], marker='o', s=100, label='Class +1')
plt.scatter(class_neg['X1'], class_neg['X2'], marker='x', s=100, label='Class -1')
plt.title('Original 2D Data (Not Linearly Separable)')
plt.xlabel('X₁')
plt.ylabel('X₂')
plt.axhline(0, color='black',linewidth=0.5)
plt.axvline(0, color='black',linewidth=0.5)
plt.grid(True)

# Plot a potential non-linear boundary (x₁x₂ = 0)
x_curve = np.linspace(-2, 2, 400)
plt.plot(x_curve, np.zeros_like(x_curve), 'k--', label='Non-linear Boundary (x₁x₂=0)')
plt.plot(np.zeros_like(x_curve), x_curve, 'k--')

plt.legend()
# plt.show()
plt.savefig("_q1_plot.png")


# --- Task 1: Define the explicit feature mapping φ(x) ---
# Derived from K(x,z) = (xᵀz + 1)² = (x₁z₁ + x₂z₂ + 1)²
def phi(x):
    """Maps a 2D vector x to the 6D feature space."""
    x1, x2 = x
    return np.array([
        x1**2,          # z₁
        np.sqrt(2) * x1 * x2, # z₂
        x2**2,          # z₃
        np.sqrt(2) * x1,    # z₄
        np.sqrt(2) * x2,    # z₅
        1               # z₆
    ])

# --- Task 2: Map all points into the new feature space ---
X = df[['X1', 'X2']].values
y = df['Class'].values
mapped_points = [phi(p) for p in X]

# Display the mapped points in a table
mapped_df = pd.DataFrame(mapped_points, columns=['z₁=x₁²', 'z₂=√2x₁x₂', 'z₃=x₂²', 'z₄=√2x₁', 'z₅=√2x₂', 'z₆=1'])
mapped_df['Class'] = y
print("--- Points in the Transformed 6D Feature Space (φ-space) ---")
print(mapped_df.to_string())


# --- Task 3 & 4: Show linear separability and find a hyperplane ---
# By inspection, the z₂ component (√2 * x₁ * x₂) is non-negative for Class -1 and negative for Class +1.
# This means a hyperplane that only uses the z₂ axis can separate the data.
# Let's define one such hyperplane: -z₂ = 0
# W = [0, -1, 0, 0, 0, 0], B = 0
W = np.array([0, -1, 0, 0, 0, 0])
B = 0.0

print("\n--- Testing Separating Hyperplane Wᵀz + B = 0 (where W=[0,-1,0,0,0,0], B=0) ---")
print(f"Hyperplane equation in φ-space: -z₂ = 0")

for i, z in enumerate(mapped_points):
    classification = np.dot(W, z) + B
    original_point = X[i]
    point_class = y[i]
    status = "Correct" if np.sign(classification) == point_class or (classification == 0 and point_class == -1) else "Incorrect"
    print(f"Point {original_point}: Wᵀz+B = {classification:.4f}. Predicted sign: {np.sign(classification):.0f}. Actual Class: {point_class}. Status: {status}")

# --- Task 5 is theoretical and explained in the summary ---