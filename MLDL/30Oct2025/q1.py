import numpy as np
import pandas as pd

df = pd.read_csv("_q1_dataset.csv")

# Separate features and labels
X = df[['Feature1', 'Feature2']].to_numpy()
y = df['Class'].to_numpy()
classes = np.unique(y)

# Step 1: Compute class-wise mean vectors
mean_vectors = {}
for c in classes:
    mean_vectors[c] = np.mean(X[y == c], axis=0)

print("Class-wise mean vectors:")
for c, mv in mean_vectors.items():
    print(f"{c}: {mv}")

# Step 2: Compute within-class scatter matrix Sw
Sw = np.zeros((2, 2))
for c in classes:
    class_scatter = np.zeros((2, 2))
    for row in X[y == c]:
        row, mv = row.reshape(2, 1), mean_vectors[c].reshape(2, 1)
        class_scatter += (row - mv) @ (row - mv).T
    Sw += class_scatter

print("\nWithin-class scatter matrix (Sw):\n", Sw)

# Compute overall mean
overall_mean = np.mean(X, axis=0).reshape(2, 1)

# Step 2b: Compute between-class scatter matrix Sb
Sb = np.zeros((2, 2))
for c in classes:
    n = X[y == c].shape[0]
    mv = mean_vectors[c].reshape(2, 1)
    Sb += n * (mv - overall_mean) @ (mv - overall_mean).T

print("\nBetween-class scatter matrix (Sb):\n", Sb)

# Step 3: Eigen decomposition of inv(Sw) @ Sb
eig_vals, eig_vecs = np.linalg.eig(np.linalg.inv(Sw).dot(Sb))

print("\nEigenvalues:\n", eig_vals)
print("\nEigenvectors:\n", eig_vecs)

# Step 4: Select top discriminant component
# Sort by eigenvalue magnitude (descending)
sorted_indices = np.argsort(abs(eig_vals))[::-1]
eig_vals = eig_vals[sorted_indices]
eig_vecs = eig_vecs[:, sorted_indices]

W = eig_vecs[:, 0].reshape(2, 1)  # top component
print("\nTop discriminant component:\n", W)

# Step 5: Project data
Y = X.dot(W)
print("\nProjected (transformed) dataset:\n", Y)

# Step 6: Discussion
print("""
LDA vs PCA:
------------
- LDA is a *supervised* technique: it uses class labels to maximize separability between classes.
- PCA is an *unsupervised* technique: it ignores labels and maximizes total variance in data.
- LDA finds a projection that best separates classes.
- PCA finds a projection that captures the most variance, which might not correspond to class separation.
""")
