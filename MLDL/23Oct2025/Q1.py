import numpy as np
import pandas as pd

# Step 1: Create the dataset
data = {
    'ID': [1, 2, 3, 4, 5],
    'Feature1': [1.0, 1.5, 2.0, 3.0, 5.0],
    'Feature2': [2.1, 1.8, 2.5, 3.5, 8.0],
    'Class': ['A', 'A', 'B', 'B', 'C']
}

df = pd.DataFrame(data)
print("Original Dataset:\n", df, "\n")

# Extract only features
X = df[['Feature1', 'Feature2']].values

# Step 1: Standardize the features
mean = np.mean(X, axis=0)
std = np.std(X, axis=0, ddof=1)
X_std = (X - mean) / std

print("Standardized Data:\n", X_std, "\n")

# Step 2: Compute covariance matrix
cov_matrix = np.cov(X_std, rowvar=False)
print("Covariance Matrix:\n", cov_matrix, "\n")

# Step 3: Compute eigenvalues and eigenvectors
eig_vals, eig_vecs = np.linalg.eig(cov_matrix)

print("Eigenvalues:\n", eig_vals)
print("Eigenvectors:\n", eig_vecs, "\n")

# Step 4: Sort eigenvalues and eigenvectors in descending order
sorted_indices = np.argsort(eig_vals)[::-1]
eig_vals = eig_vals[sorted_indices]
eig_vecs = eig_vecs[:, sorted_indices]

# Calculate explained variance ratio
explained_variance_ratio = eig_vals / np.sum(eig_vals)

print("Explained Variance Ratio:\n", explained_variance_ratio, "\n")

# Step 5: Project data onto the first principal component
pc1 = eig_vecs[:, 0]  # first principal component
X_pca = X_std.dot(pc1.reshape(-1, 1))

df_transformed = pd.DataFrame({
    'ID': df['ID'],
    'PC1': X_pca.flatten(),
    'Class': df['Class']
})

print("Data projected onto the first principal component:\n", df_transformed, "\n")

# Step 6: Discussion
print("Discussion:")
print("""
- PCA finds directions (principal components) that capture the maximum variance in data.
- Here, the first principal component explains most of the variance (~depends on eigenvalue ratio).
- Projecting data onto PC1 reduces 2D data to 1D while preserving most of its information.
- This helps in visualization, compression, and removing redundancy.
""")
