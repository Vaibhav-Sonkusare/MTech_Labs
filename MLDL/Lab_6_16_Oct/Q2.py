import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# Create the dataset and standardize it manually
data = {
    "Feature1": [4.0, 2.0, 3.0, 5.0, 6.0],
    "Feature2": [2.0, 3.5, 4.0, 3.0, 5.0],
    "Feature3": [0.6, 1.1, 0.9, 1.0, 1.5],
    "Feature4": [3.0, 2.2, 3.5, 4.0, 5.0]
}

df = pd.DataFrame(data, index=[1, 2, 3, 4, 5])
print("Original Dataset:\n", df, "\n")

# Standardization: mean = 0, variance = 1
mean = df.mean()
std = df.std(ddof=0)
standardized_df = (df - mean) / std
print("Standardized Dataset:\n", standardized_df, "\n")

# Compute covariance matrix, eigenvalues, and eigenvectors
cov_matrix = np.cov(standardized_df.T)
print("Covariance Matrix:\n", cov_matrix, "\n")

# Eigen decomposition
eigenvalues, eigenvectors = np.linalg.eig(cov_matrix)

# Sort eigenvalues & eigenvectors in descending order
sorted_indices = np.argsort(eigenvalues)[::-1]
eigenvalues = eigenvalues[sorted_indices]
eigenvectors = eigenvectors[:, sorted_indices]

# Explained variance
explained_variance_ratio = eigenvalues / np.sum(eigenvalues)

print("Eigenvalues:\n", eigenvalues)
print("\nEigenvectors (columns are PCs):\n", eigenvectors)
print("\nExplained Variance Ratio (%):\n", explained_variance_ratio * 100, "\n")

# Project data into PCA space (PC1 vs PC2)
# Get first two eigenvectors
W = eigenvectors[:, :2]

# Transform data
pca_data = standardized_df.values.dot(W)

plt.figure(figsize=(8,6))
plt.scatter(pca_data[:, 0], pca_data[:, 1], color='steelblue', s=80)
plt.xlabel(f"PC1 ({explained_variance_ratio[0]*100:.2f}% variance)")
plt.ylabel(f"PC2 ({explained_variance_ratio[1]*100:.2f}% variance)")
plt.title("PCA (Manual): PC1 vs PC2")

# Draw feature vectors to show direction of max variance
for i, feature in enumerate(df.columns):
    plt.arrow(0, 0, eigenvectors[i,0]*2, eigenvectors[i,1]*2, 
              color='r', head_width=0.05, alpha=0.6)
    plt.text(eigenvectors[i,0]*2.2, eigenvectors[i,1]*2.2, feature, color='r')

plt.grid(True)
# plt.show()
plt.savefig("_q2_fig.png")

# Interpretation
# Top 2 contributing features for PC1
loadings_pc1 = np.abs(eigenvectors[:, 0])
top_features_pc1 = df.columns[np.argsort(loadings_pc1)[-2:][::-1]]

print("Top 2 features contributing most to PC1:", list(top_features_pc1))
print("Total variance captured by first two PCs: %.2f%%" % (np.sum(explained_variance_ratio[:2]) * 100))
