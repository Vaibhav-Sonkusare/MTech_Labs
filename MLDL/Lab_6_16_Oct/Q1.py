import numpy as np
import pandas as pd

# Step 1: Load dataset
data = pd.DataFrame({
    'ID': [1, 2, 3, 4, 5],
    'Feature1': [2.5, 0.5, 2.2, 1.9, 3.1],
    'Feature2': [2.4, 0.7, 2.9, 2.2, 3.0],
    'Feature3': [1.0, 2.0, 2.1, 3.0, 1.1]
})

X = data[['Feature1', 'Feature2', 'Feature3']].values
print("Original Data:\n", X)

# Step 2: Standardize the dataset (mean = 0, std = 1)
means = np.mean(X, axis=0)
stds = np.std(X, axis=0, ddof=1)  # sample std (ddof=1)
X_std = (X - means) / stds

print("\nStandardized Data (mean=0, std=1):\n", X_std)

# Step 3: Compute the covariance matrix
cov_matrix = np.cov(X_std.T)
print("\nCovariance Matrix:\n", cov_matrix)

# Step 4: Compute eigenvalues and eigenvectors
eigenvalues, eigenvectors = np.linalg.eig(cov_matrix)

# Sort eigenvalues and corresponding eigenvectors in descending order
sorted_indices = np.argsort(eigenvalues)[::-1]
eigenvalues = eigenvalues[sorted_indices]
eigenvectors = eigenvectors[:, sorted_indices]

print("\nEigenvalues (sorted):\n", eigenvalues)
print("\nEigenvectors (sorted):\n", eigenvectors)

# Step 5: Compute explained variance ratio
explained_variance_ratio = eigenvalues / np.sum(eigenvalues)
print("\nExplained Variance Ratio:\n", explained_variance_ratio)

# Step 6: Project data onto the first two principal components
pc1 = eigenvectors[:, 0]
pc2 = eigenvectors[:, 1]
X_pca = X_std @ np.column_stack((pc1, pc2))

print("\nProjected Data onto first two Principal Components:\n", X_pca)

# Step 7: Discuss variance explained
print(f"\nVariance explained by PC1: {explained_variance_ratio[0]*100:.2f}%")
print(f"Variance explained by PC2: {explained_variance_ratio[1]*100:.2f}%")
print(f"Total (PC1 + PC2): {(explained_variance_ratio[0] + explained_variance_ratio[1])*100:.2f}%")
