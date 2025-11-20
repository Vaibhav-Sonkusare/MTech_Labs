import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from sklearn.svm import SVC

# --- Dataset ---
data = {
    'X1': [0, 2, 0, 2],
    'X2': [0, 2, 2, 0],
    'Class': [1, 1, -1, -1]
}
df = pd.DataFrame(data)
X = df[['X1', 'X2']].values
y = df['Class'].values
point_labels = ['x₁=(0,0)', 'x₂=(2,2)', 'x₃=(0,2)', 'x₄=(2,0)']

# --- RBF Kernel Function ---
def rbf_kernel(x, z, gamma):
    """Computes the RBF kernel value between two points."""
    distance_sq = np.sum((x - z) ** 2)
    return np.exp(-gamma * distance_sq)

def compute_kernel_matrix(X, gamma, labels):
    """Computes the pairwise kernel similarity matrix for all points."""
    n_samples = X.shape[0]
    kernel_matrix = np.zeros((n_samples, n_samples))
    for i in range(n_samples):
        for j in range(n_samples):
            kernel_matrix[i, j] = rbf_kernel(X[i], X[j], gamma)
    return pd.DataFrame(kernel_matrix, index=labels, columns=labels)

# --- Task 1: Compute Kernel Matrix for γ = 0.5 ---
gamma_low = 0.5
kernel_matrix_low_gamma = compute_kernel_matrix(X, gamma_low, point_labels)
print(f"--- Task 1: Pairwise Kernel Similarities for γ = {gamma_low} ---")
print(kernel_matrix_low_gamma.round(4))

# --- Task 2: Compute Kernel Matrix for γ = 5 ---
gamma_high = 5.0
kernel_matrix_high_gamma = compute_kernel_matrix(X, gamma_high, point_labels)
print(f"\n--- Task 2: Pairwise Kernel Similarities for γ = {gamma_high} ---")
print(kernel_matrix_high_gamma.round(4))

# --- Task 3: Visualize the effect of γ on the decision boundary ---
# Create a mesh to plot in
h = .02  # step size in the mesh
x_min, x_max = X[:, 0].min() - 1, X[:, 0].max() + 1
y_min, y_max = X[:, 1].min() - 1, X[:, 1].max() + 1
xx, yy = np.meshgrid(np.arange(x_min, x_max, h), np.arange(y_min, y_max, h))

# Create two SVM classifiers with different gammas
clf_low_gamma = SVC(kernel='rbf', gamma=gamma_low).fit(X, y)
clf_high_gamma = SVC(kernel='rbf', gamma=gamma_high).fit(X, y)

# Plotting
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

# Plot for γ = 0.5
Z1 = clf_low_gamma.decision_function(np.c_[xx.ravel(), yy.ravel()])
Z1 = Z1.reshape(xx.shape)
ax1.contourf(xx, yy, Z1, cmap=plt.cm.coolwarm, alpha=0.8)
ax1.scatter(X[:, 0], X[:, 1], c=y, cmap=plt.cm.coolwarm, s=100, edgecolors='k')
ax1.set_title(f'Decision Boundary for γ = {gamma_low} (Smoother)')
ax1.set_xlabel('X₁'); ax1.set_ylabel('X₂')

# Plot for γ = 5
Z2 = clf_high_gamma.decision_function(np.c_[xx.ravel(), yy.ravel()])
Z2 = Z2.reshape(xx.shape)
ax2.contourf(xx, yy, Z2, cmap=plt.cm.coolwarm, alpha=0.8)
ax2.scatter(X[:, 0], X[:, 1], c=y, cmap=plt.cm.coolwarm, s=100, edgecolors='k')
ax2.set_title(f'Decision Boundary for γ = {gamma_high} (Sharper/Overfit)')
ax2.set_xlabel('X₁'); ax2.set_ylabel('X₂')

plt.savefig("_q2_plot_gamma_effect.png")
