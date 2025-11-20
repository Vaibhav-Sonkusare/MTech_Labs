import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# Step 1: Create dataset
data = {
    'Customer_ID': [1, 2, 3, 4, 5],
    'Purchase_Frequency': [5, 3, 8, 2, 6],
    'Avg_Spending': [200, 150, 300, 100, 250],
    'Discount_Used': [0.2, 0.5, 0.1, 0.8, 0.3],
    'Visits_Per_Month': [3, 4, 5, 2, 6]
}

df = pd.DataFrame(data)

# Step 2: Standardize numerical features (mean = 0, std = 1)
features = ['Purchase_Frequency', 'Avg_Spending', 'Discount_Used', 'Visits_Per_Month']
X = df[features].values

means = np.mean(X, axis=0)
stds = np.std(X, axis=0, ddof=1)
X_std = (X - means) / stds

# Step 3: Covariance matrix
cov_matrix = np.cov(X_std, rowvar=False)

# Step 4: Eigen decomposition
eigenvalues, eigenvectors = np.linalg.eig(cov_matrix)

# Step 5: Sort eigenvalues and eigenvectors
idx = np.argsort(eigenvalues)[::-1]
eigenvalues = eigenvalues[idx]
eigenvectors = eigenvectors[:, idx]

# Step 6: Explained variance
explained_variance_ratio = eigenvalues / np.sum(eigenvalues)

# Step 7: Project data onto top 2 principal components
X_pca = np.dot(X_std, eigenvectors[:, :2])
print("______________1:", X_pca)

# Step 8: Plot customers in PC1–PC2 space
spending_level = df['Avg_Spending']
print("______________2:", spending_level)

plt.figure(figsize=(8,6))
plt.scatter(X_pca[:, 0], X_pca[:, 1],
            c=spending_level, cmap='viridis', s=100, edgecolors='k')

for i, cid in enumerate(df['Customer_ID']):
    plt.text(X_pca[i, 0] + 0.05, X_pca[i, 1], f'Cust {cid}', fontsize=9)

plt.xlabel('Principal Component 1')
plt.ylabel('Principal Component 2')
plt.title('PCA on Customer Purchase Behavior')
plt.colorbar(label='Avg Spending')
plt.grid(True)
# plt.show()
plt.savefig("_Q2_fig.jpeg")

# Step 9: Print results
print("Eigenvalues:\n", eigenvalues)
print("\nEigenvectors (columns correspond to PCs):\n", eigenvectors)
print("\nExplained variance ratio:\n", explained_variance_ratio)

# Interpretation hints
loadings = pd.DataFrame(eigenvectors, index=features, columns=['PC1', 'PC2', 'PC3', 'PC4'])
print("\nFeature loadings on PCs:\n", loadings)
