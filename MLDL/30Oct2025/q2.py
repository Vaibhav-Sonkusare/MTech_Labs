import numpy as np
import pandas as pd

# Step 1: Define data matrix
data = pd.read_csv("_q2_dataset.csv")
X = data.to_numpy()

print("Original Data Matrix (X):\n", X)

# Step 2: Center the data by subtracting column means
X_mean = np.mean(X, axis=0)
X_centered = X - X_mean
print("\nColumn Means:\n", X_mean)
print("\nCentered Data Matrix (X_centered):\n", X_centered)

# Step 3: Compute SVD
U, S, Vt = np.linalg.svd(X_centered, full_matrices=False)
print("\nU Matrix:\n", U)
print("\nSingular Values (S):\n", S)
print("\nV Transpose (Vt):\n", Vt)

# Step 4: Interpret singular values
total_variance = np.sum(S**2)
variance_ratio = (S**2) / total_variance
print("\nVariance captured by each singular value:\n", variance_ratio)

# Step 5: Reconstruct using top 2 singular values
k = 2
X_approx = U[:, :k] @ np.diag(S[:k]) @ Vt[:k, :]
print("\nReconstructed Data using top 2 singular values:\n", X_approx + X_mean)

# Step 6: Compare original and reconstructed
reconstruction_error = np.linalg.norm(X - (X_approx + X_mean))
print("\nReconstruction Error (Frobenius norm):", reconstruction_error)
