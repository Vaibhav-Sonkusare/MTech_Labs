import numpy as np
import matplotlib.pyplot as plt

# ===============================
#  DATASET
# ===============================
X = np.array([
    [1.0, 1.2],
    [1.3, 1.0],
    [2.8, 3.0],
    [3.2, 3.4],
    [1.1, 1.3]
])
y = np.array([0, 0, 1, 1, 0])

# Convert labels for perceptron: 0 → -1, 1 → +1
y_p = np.where(y == 1, 1, -1)

# ===============================
# 1. PERCEPTRON TRAINING
# ===============================
w = np.zeros(3)        # [bias, w1, w2]
lr = 1.0               # learning rate

def perceptron_predict(x, w):
    return 1 if np.dot(w, x) >= 0 else -1

# Training loop
for epoch in range(20):  
    for xi, target in zip(X, y_p):
        xi_aug = np.insert(xi, 0, 1)   # add bias term
        pred = perceptron_predict(xi_aug, w)
        if pred != target:
            w += lr * target * xi_aug

print("Final perceptron weights:", w)

# ===============================
# 2. GAUSSIAN GENERATIVE CLASSIFIER
# ===============================
class0 = X[y == 0]
class1 = X[y == 1]

mu0 = class0.mean(axis=0)
mu1 = class1.mean(axis=0)

# Shared covariance matrix
S = np.cov(X.T)
Sinv = np.linalg.inv(S)

# Decision function (quadratic discriminant)
def quad(x, mu):
    d = x - mu
    return np.sum((d @ Sinv) * d, axis=1)

# Generate grid for decision boundary
x_min, x_max = 0.5, 3.5
y_min, y_max = 0.5, 3.8
xx, yy = np.meshgrid(np.linspace(x_min, x_max, 300),
                     np.linspace(y_min, y_max, 300))
grid = np.c_[xx.ravel(), yy.ravel()]

g0 = quad(grid, mu0)
g1 = quad(grid, mu1)
boundary = (g0 - g1).reshape(xx.shape)

# ===============================
# PLOT: Gaussian Generative Boundary
# ===============================
plt.figure()
plt.contour(xx, yy, boundary, levels=[0])
plt.scatter(class0[:,0], class0[:,1], label="Class 0")
plt.scatter(class1[:,0], class1[:,1], label="Class 1")
plt.title("Gaussian Generative Classifier Decision Boundary")
plt.xlabel("Feature 1")
plt.ylabel("Feature 2")
plt.legend()
# plt.show()
plt.savefig("fig1-1.png")

# ===============================
# PLOT: Perceptron Linear Boundary
# ===============================
plt.figure()
x_line = np.linspace(x_min, x_max, 200)
y_line = -(w[0] + w[1]*x_line) / w[2]

plt.plot(x_line, y_line, label="Perceptron Boundary")
plt.scatter(class0[:,0], class0[:,1])
plt.scatter(class1[:,0], class1[:,1])
plt.xlabel("Feature 1")
plt.ylabel("Feature 2")
plt.title("Perceptron Linear Decision Boundary")
plt.legend()
# plt.show()
plt.savefig("fig1-2.png")

# ===============================
# 3. LIMITATION OF FIXED BASIS FUNCTIONS (XOR EXAMPLE)
# ===============================
xor_X = np.array([[0,0], [0,1], [1,0], [1,1]])
xor_y = np.array([0,1,1,0])

plt.figure()
plt.scatter(xor_X[xor_y==0][:,0], xor_X[xor_y==0][:,1], label="Class 0")
plt.scatter(xor_X[xor_y==1][:,0], xor_X[xor_y==1][:,1], label="Class 1")
plt.title("XOR – Example of Non-linearly Separable Data")
plt.xlabel("x1")
plt.ylabel("x2")
plt.legend()
# plt.show()
plt.savefig("fig1-3.png")