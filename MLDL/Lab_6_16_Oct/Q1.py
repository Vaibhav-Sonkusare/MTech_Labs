import numpy as np
import pandas as pd

df = pd.read_csv("_q1_dataset.csv")
data = df[['Feature1', 'Feature2', 'Feature3']].to_numpy()
print("Original Dataset:")
print(data)

means = data.mean(axis=0)
std = data.std(axis=0)
sdata = (data - data.mean(axis=0)) / data.std(axis=0, ddof=1)
print("Standardized Dataset:")
print(sdata)

# Verifying mean and standerd deviation of sdata
# print(np.mean(sdata, axis=0))
# print(np.std(sdata, axis=0, ddof=1))

cov_sdata = np.cov(sdata.T)
print("Covariance Matrix:")
print(cov_sdata)

e_value, e_vector = np.linalg.eig(cov_sdata)
print("Eigen Values and Eigen Vectors:")
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
X_pca = std @ np.column_stack((pc1, pc2))
# print("New Datase:")
# print(X_pca)

explained_variance_ratio = e_value / np.sum(e_value)
print("\nExplained Variance Ratio:\n", explained_variance_ratio)

print("\nProjected Data onto first two Principal Components:\n", X_pca)

# Step 7: Discuss variance explained
print(f"\nVariance explained by PC1: {explained_variance_ratio[0]*100:.2f}%")
print(f"Variance explained by PC2: {explained_variance_ratio[1]*100:.2f}%")
print(f"Total (PC1 + PC2): {(explained_variance_ratio[0] + explained_variance_ratio[1])*100:.2f}%")
