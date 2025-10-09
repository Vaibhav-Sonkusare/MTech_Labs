import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from sklearn.svm import SVC

# --- Dataset ---
X = np.array([[1, 0], [-1, 0], [0, 1], [0, -1]])
y = np.array([1, 1, -1, -1])

# Create a mesh to plot in
h = .02
x_min, x_max = X[:, 0].min() - 1, X[:, 0].max() + 1
y_min, y_max = X[:, 1].min() - 1, X[:, 1].max() + 1
xx, yy = np.meshgrid(np.arange(x_min, x_max, h), np.arange(y_min, y_max, h))

# --- Classifiers ---
# Polynomial Kernel of degree 2
clf_poly = SVC(kernel='poly', degree=2, coef0=1, C=100).fit(X, y) # coef0=1 to match (xᵀz+1)²
# RBF Kernel with gamma=1
clf_rbf = SVC(kernel='rbf', gamma=1, C=100).fit(X, y)

# --- Plotting ---
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

# Plot Polynomial Boundary
Z1 = clf_poly.predict(np.c_[xx.ravel(), yy.ravel()]).reshape(xx.shape)
ax1.contourf(xx, yy, Z1, cmap=plt.cm.coolwarm, alpha=0.8)
ax1.scatter(X[:, 0], X[:, 1], c=y, cmap=plt.cm.coolwarm, s=100, edgecolors='k')
ax1.set_title('Polynomial Kernel (degree 2) Boundary')
ax1.set_xlabel('X₁'); ax1.set_ylabel('X₂')

# Plot RBF Boundary
Z2 = clf_rbf.predict(np.c_[xx.ravel(), yy.ravel()]).reshape(xx.shape)
ax2.contourf(xx, yy, Z2, cmap=plt.cm.coolwarm, alpha=0.8)
ax2.scatter(X[:, 0], X[:, 1], c=y, cmap=plt.cm.coolwarm, s=100, edgecolors='k')
ax2.set_title('RBF Kernel (γ = 1) Boundary')
ax2.set_xlabel('X₁'); ax2.set_ylabel('X₂')

plt.savefig("_q3_kernel_comparison.png")
