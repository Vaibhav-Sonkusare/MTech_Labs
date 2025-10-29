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
plt.axhline(0, color='black', linewidth=0.5)
plt.axvline(0, color='black', linewidth=0.5)
plt.grid(True)

# Show potential non-linear boundaries (x₁x₂ = 0)
x_curve = np.linspace(-2, 2, 400)
plt.plot(x_curve, np.zeros_like(x_curve), 'k--', label='x₁x₂ = 0 boundary')
plt.plot(np.zeros_like(x_curve), x_curve, 'k--')

plt.legend()
plt.savefig("_Q1_plot.png")
# plt.show()


# --- Task 1: Explicit feature mapping φ(x) ---
def phi(x):
    """Maps 2D input x to the 6D feature space."""
    x1, x2 = x
    return np.array([
        x1**2,                 # z₁ = x₁²
        np.sqrt(2) * x1 * x2,  # z₂ = √2 * x₁x₂
        x2**2,                 # z₃ = x₂²
        np.sqrt(2) * x1,       # z₄ = √2 * x₁
        np.sqrt(2) * x2,       # z₅ = √2 * x₂
        1                      # z₆ = 1
    ])

print("\n--- Task 1: Feature Mapping φ(x) ---")
print("φ(x) = (x₁², √2 x₁x₂, x₂², √2x₁, √2x₂, 1)")


# --- Task 2: Map all points into the feature space ---
X = df[['X1', 'X2']].values
y = df['Class'].values
mapped_points = [phi(p) for p in X]

mapped_df = pd.DataFrame(mapped_points, columns=['z₁=x₁²', 'z₂=√2x₁x₂', 'z₃=x₂²', 'z₄=√2x₁', 'z₅=√2x₂', 'z₆=1'])
mapped_df['Class'] = y
print("\n--- Task 2: Points in the Transformed 6D Feature Space ---")
print(mapped_df.to_string(index=False))


# --- Task 3: Show linear separability ---
print("\n--- Task 3: Verifying Linear Separability ---")
print("Observe that z₂ (√2 x₁x₂) separates classes:")

for i, z in enumerate(mapped_points):
    print(f"Point {X[i]} → z₂ = {z[1]:.4f}, Class = {y[i]}")


# --- Task 4: Find a separating hyperplane ---
print("\n--- Task 4: One Possible Separating Hyperplane ---")
print("Using only z₂ component:")
print("Hyperplane: -z₂ = 0")
print("Weights: W = [0, -1, 0, 0, 0, 0], Bias: B = 0")

W = np.array([0, -1, 0, 0, 0, 0])
B = 0.0

for i, z in enumerate(mapped_points):
    decision_value = np.dot(W, z) + B
    predicted = np.sign(decision_value) if decision_value != 0 else -1
    correctness = "Correct" if predicted == y[i] else "Incorrect"
    print(f"Point {X[i]} → Decision = {decision_value:.4f}, Predicted = {predicted}, Actual = {y[i]} → {correctness}")


# --- Task 5: Decision function using kernel trick ---
print("\n--- Task 5: Decision Function Using Kernel Trick ---")
print("Kernel: K(x, z) = (xᵀz + 1)²")
print("Decision function:")
print("f(x) = sign( - √2 * x₁ * x₂ )")
print("or equivalently:")
print("f(x) = sign( -K(x, [1,1]) + K(x, [-1,1]) + K(x, [1,-1]) ) / appropriate scaling (depending on formulation)")

print("\nThis shows that the classifier can be implemented directly using the kernel without computing φ(x).")
