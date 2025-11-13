"""
1. Implement a 2-2-1 Multilayer Network for the XOR dataset and perform forward computation for all samples.
2. Using the same network, apply back propagation for one training epoch and update the weights.
3. Modify the model by adding a residual connection (skip connection) and observe how the output changes.
"""

import numpy as np

# XOR dataset
X = np.array([
    [0,0],
    [0,1],
    [1,0],
    [1,1]
], dtype=float)

y = np.array([[0],[1],[1],[0]], dtype=float)

# ----- Utility Functions -----
def sigmoid(x):
    return 1 / (1 + np.exp(-x))

def sigmoid_deriv(x):
    return sigmoid(x) * (1 - sigmoid(x))

# ============================================================
# PART 1: 2-2-1 MLP – Forward Computation
# ============================================================

# Initialize weights
np.random.seed(42)
W1 = np.random.randn(2, 2)     # input -> hidden
b1 = np.random.randn(1, 2)
W2 = np.random.randn(2, 1)     # hidden -> output
b2 = np.random.randn(1, 1)

print("Initial Weights:")
print("W1:\n", W1)
print("b1:\n", b1)
print("W2:\n", W2)
print("b2:\n", b2)

def forward_pass(X):
    z1 = X @ W1 + b1
    a1 = sigmoid(z1)
    z2 = a1 @ W2 + b2
    a2 = sigmoid(z2)
    return z1, a1, z2, a2

print("\n=== PART 1: Forward Pass Outputs ===")
z1, a1, z2, a2 = forward_pass(X)
print("Hidden layer activations:\n", a1)
print("Output predictions:\n", a2)

# ============================================================
# PART 2: Backpropagation for 1 Training Epoch
# ============================================================

lr = 0.1  # learning rate

# Forward
z1, a1, z2, a2 = forward_pass(X)

# Compute error
error = a2 - y

# Gradients for output layer
dZ2 = error * sigmoid_deriv(z2)
dW2 = a1.T @ dZ2
db2 = np.sum(dZ2, axis=0, keepdims=True)

# Gradients for hidden layer
dA1 = dZ2 @ W2.T
dZ1 = dA1 * sigmoid_deriv(z1)
dW1 = X.T @ dZ1
db1 = np.sum(dZ1, axis=0, keepdims=True)

# Update weights
W1 -= lr * dW1
b1 -= lr * db1
W2 -= lr * dW2
b2 -= lr * db2

print("\n=== PART 2: Updated Weights After Backprop ===")
print("W1:\n", W1)
print("b1:\n", b1)
print("W2:\n", W2)
print("b2:\n", b2)

# ============================================================
# PART 3: Add a Residual (Skip) Connection
# ============================================================

def forward_with_residual(X):
    """Adds skip connection from input → output layer."""
    z1 = X @ W1 + b1
    a1 = sigmoid(z1)

    # MLP output
    z2 = a1 @ W2 + b2
    a2 = sigmoid(z2)

    # Residual (skip): add transformed input
    # To match output dimension (1), use weighted sum of input
    Wr = np.array([[0.6],[0.6]])  # Residual weights

    residual = X @ Wr
    out = a2 + residual
    return out

print("\n=== PART 3: Output With a Residual Connection ===")
print(forward_with_residual(X))
