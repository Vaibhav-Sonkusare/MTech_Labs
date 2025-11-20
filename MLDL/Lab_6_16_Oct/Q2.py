import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("_q2_dataset.csv")
data = df.to_numpy()
print("Original Data:")
print(data)

sdata = (data - data.mean(axis=0)) / data.std(axis=0, ddof=1)
print("Standardized Data:")
print(sdata)

# Covariance matrix
cov_sdata = np.cov(sdata.T)

e_value, e_vector = np.linalg.eig(cov_sdata)
print("Eigen Value and Eigen Vectors:")
print(e_value)
print(e_vector)

sorted_indices = np.argsort(e_value)[::-1]
e_value = e_value[sorted_indices]
e_vector = e_vector[:, sorted_indices]
print("Eigen Values and vectors sorted in decreasing order are:")
print(e_value)
print(e_vector)

pc1 = e_vector[:, 0]
pc2 = e_vector[:, 1]
X_pca = data.std(axis=0, ddof=1) @ np.column_stack((pc1, pc2))
# print("New Datase:")
# print(X_pca)

explained_variance_ratio = e_value / np.sum(e_value)
print("\nExplained Variance Ratio:\n", explained_variance_ratio)

print("\nProjected Data onto first two Principal Components:\n", X_pca)

# Step 7: Discuss variance explained
print(f"\nVariance explained by PC1: {explained_variance_ratio[0]*100:.2f}%")
print(f"Variance explained by PC2: {explained_variance_ratio[1]*100:.2f}%")
# print(f"Total (PC1 + PC2): {(explained_variance_ratio[0] + explained_variance_ratio[1])*100:.2f}%")

W = e_vector[:, :2]

# Transform data
pca_data = sdata.values.dot(W)

plt.figure(figsize=(8,6))
plt.scatter(pca_data[:, 0], pca_data[:, 1], color='steelblue', s=80)
plt.xlabel(f"PC1 ({explained_variance_ratio[0]*100:.2f}% variance)")
plt.ylabel(f"PC2 ({explained_variance_ratio[1]*100:.2f}% variance)")
plt.title("PCA (Manual): PC1 vs PC2")

# Draw feature vectors to show direction of max variance
for i, feature in enumerate(df.columns):
    plt.arrow(0, 0, e_vector[i,0]*2, e_vector[i,1]*2, 
              color='r', head_width=0.05, alpha=0.6)
    plt.text(e_vector[i,0]*2.2, e_vector[i,1]*2.2, feature, color='r')

plt.grid(True)
# plt.show()
plt.savefig("_q2_fig.png")

# Interpretation
# Top 2 contributing features for PC1
loadings_pc1 = np.abs(e_vector[:, 0])
top_features_pc1 = df.columns[np.argsort(loadings_pc1)[-2:][::-1]]

print("Top 2 features contributing most to PC1:", list(top_features_pc1))
print("Total variance captured by first two PCs: %.2f%%" % (np.sum(explained_variance_ratio[:2]) * 100))
