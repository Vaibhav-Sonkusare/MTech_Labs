"""
1. Train a neural network using gradient descent with two different learning rates and compare the convergence curves.
2. Apply L2 weight decay regularization and observe how the learned decision boundary differes from the non-regularized model.
3. Plot learning curves (training vs. validation loss) and explain the inductive bias and generalization behaviour of the model.
"""

import numpy as np
import matplotlib.pyplot as plt

# ==========================
# Dataset
# ==========================
X = np.array([
    [1.0, 1.1],
    [1.2, 0.9],
    [3.0, 3.2],
    [3.4, 3.1],
    [2.5, 2.8]
])

np.random.seed(42)

y = np.array([0, 0, 1, 1, 1]).reshape(-1, 1)

# Train–validation split
X_train, X_val = X[:4], X[4:].copy()
y_train, y_val = y[:4], y[4:].copy()

# ==========================
# Helper functions
# ==========================
def sigmoid(z):
    return 1 / (1 + np.exp(-z))

def compute_loss(X, y, W, b, lambd=0.0):
    m = len(y)
    z = X @ W + b
    preds = sigmoid(z)
    loss = -np.mean(y*np.log(preds + 1e-9) + (1-y)*np.log(1-preds + 1e-9))
    loss += lambd * np.sum(W**2)
    return loss

def train(X, y, lr, lambd=0.0, epochs=200):
    m, n = X.shape
    W = np.zeros((n, 1))
    b = 0
    losses = []

    for _ in range(epochs):
        z = X @ W + b
        preds = sigmoid(z)

        dW = (X.T @ (preds - y)) / m + 2*lambd*W
        db = np.mean(preds - y)

        W -= lr * dW
        b -= lr * db

        losses.append(compute_loss(X, y, W, b, lambd))

    return W, b, losses

# ==========================
# Train on two learning rates
# ==========================
W_lr1, b_lr1, losses_lr1 = train(X_train, y_train, lr=0.1)
W_lr2, b_lr2, losses_lr2 = train(X_train, y_train, lr=0.01)

# ==========================
# Train with/without L2 regularization
# ==========================
W_nr, b_nr, _ = train(X_train, y_train, lr=0.1, lambd=0)      # no regularization
W_reg, b_reg, _ = train(X_train, y_train, lr=0.1, lambd=0.1) # L2 regularization

# ==========================
# Validation loss curve (constant for 1 sample)
# ==========================
def compute_val_loss(W, b):
    return compute_loss(X_val, y_val, W, b)

train_W, train_b, train_curve = train(X_train, y_train, lr=0.1)
val_loss = compute_val_loss(train_W, train_b)

# ==========================
# Plot 1: Convergence curves
# ==========================
plt.figure(figsize=(10,6))
plt.plot(losses_lr1, label="Learning rate = 0.1")
plt.plot(losses_lr2, label="Learning rate = 0.01")
plt.xlabel("Epoch")
plt.ylabel("Loss")
plt.title("Convergence Curves for Two Learning Rates")
plt.legend()
plt.grid(True)
# plt.show()
plt.savefig("fig3-1.png")

# ==========================
# Helper: plot decision boundaries
# ==========================
def plot_decision_boundary(W, b, title):
    x_min, x_max = X[:,0].min()-1, X[:,0].max()+1
    y_min, y_max = X[:,1].min()-1, X[:,1].max()+1
    xx, yy = np.meshgrid(
        np.linspace(x_min, x_max, 200),
        np.linspace(y_min, y_max, 200)
    )
    Z = sigmoid(np.c_[xx.ravel(), yy.ravel()] @ W + b)
    Z = Z.reshape(xx.shape)

    plt.figure(figsize=(10,6))
    plt.contour(xx, yy, Z, levels=[0.5])
    plt.scatter(X[:,0], X[:,1], c=y.flatten(), cmap="viridis")
    plt.title(title)
    plt.xlabel("Feature 1")
    plt.ylabel("Feature 2")
    plt.grid(True)
    # plt.show()
    plt.savefig(f"fig3-2-{np.random.randint(0, 1000)}.png")

# ==========================
# Plot 2: Decision boundaries
# ==========================
plot_decision_boundary(W_nr,  b_nr,  "Decision Boundary (No Regularization)")
plot_decision_boundary(W_reg, b_reg, "Decision Boundary (L2 Regularization)")

# ==========================
# Plot 3: Learning curves
# ==========================
plt.figure(figsize=(10,6))
plt.plot(train_curve, label="Training Loss")
plt.plot([val_loss]*len(train_curve), label="Validation Loss")
plt.xlabel("Epoch")
plt.ylabel("Loss")
plt.title("Learning Curve (Training vs Validation)")
plt.legend()
plt.grid(True)
# plt.show()
plt.savefig("fig3-3.png")
