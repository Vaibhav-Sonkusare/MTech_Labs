import matplotlib.pyplot as plt
import numpy as np
from sklearn.svm import SVC, LinearSVC
from sklearn.exceptions import ConvergenceWarning
import warnings

# Suppress convergence warnings for linear SVM in this example
warnings.filterwarnings("ignore", category=ConvergenceWarning)

# --- Dataset ---
X = np.array([[1, 0], [-1, 0], [0, 1], [0, -1]])
y = np.array([1, 1, -1, -1])

# 1. Check if linearly separable
linear_clf = LinearSVC(C=1e10, max_iter=10000)
linear_clf.fit(X, y)
score = linear_clf.score(X, y)

if score < 1.0:
    print("Dataset is NOT linearly separable in original space.")
else:
    print("Dataset is linearly separable in original space.")

# Create a mesh to plot decision boundaries
h = .02
x_min, x_max = X[:, 0].min() - 1, X[:, 0].max() + 1
y_min, y_max = X[:, 1].min() - 1, X[:, 1].max() + 1
xx, yy = np.meshgrid(np.arange(x_min, x_max, h), np.arange(y_min, y_max, h))

# 2. Polynomial Kernel (degree 2)
clf_poly = SVC(kernel='poly', degree=2, coef0=1, C=100)
clf_poly.fit(X, y)

# 3. RBF Kernel with gamma=1
clf_rbf = SVC(kernel='rbf', gamma=1, C=100)
clf_rbf.fit(X, y)

# 4. Compare margins based on support vectors and dual coefficients
poly_margin = 1 / np.linalg.norm(clf_poly.coef_ if hasattr(clf_poly, 'coef_') else clf_poly.support_vectors_, axis=1).min()
rbf_margin = 1 / np.linalg.norm(clf_rbf.support_vectors_, axis=1).min()

print(f"Polynomial kernel margin estimate (approx): {poly_margin:.3f}")
print(f"RBF kernel margin estimate (approx): {rbf_margin:.3f}")

if poly_margin > rbf_margin:
    print("Polynomial kernel gives a better margin.")
else:
    print("RBF kernel gives a better margin (often better for radial symmetry).")

# --- Plotting ---
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

# Plot Polynomial Kernel Boundary
Z1 = clf_poly.predict(np.c_[xx.ravel(), yy.ravel()]).reshape(xx.shape)
ax1.contourf(xx, yy, Z1, cmap=plt.cm.coolwarm, alpha=0.8) # type: ignore
ax1.scatter(X[:, 0], X[:, 1], c=y, cmap=plt.cm.coolwarm, s=100, edgecolors='k') # type: ignore
ax1.set_title('Polynomial Kernel (degree 2)')
ax1.set_xlabel('X₁')
ax1.set_ylabel('X₂')

# Plot RBF Kernel Boundary
Z2 = clf_rbf.predict(np.c_[xx.ravel(), yy.ravel()]).reshape(xx.shape)
ax2.contourf(xx, yy, Z2, cmap=plt.cm.coolwarm, alpha=0.8) # type: ignore
ax2.scatter(X[:, 0], X[:, 1], c=y, cmap=plt.cm.coolwarm, s=100, edgecolors='k') # type: ignore
ax2.set_title('RBF Kernel (γ = 1)')
ax2.set_xlabel('X₁')
ax2.set_ylabel('X₂')

plt.tight_layout()
plt.savefig("_Q3_kernel_comparison.png")
# plt.show()
