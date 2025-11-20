# KNN Classifier

from sklearn.neighbors import KDTree
import numpy as np

# Step 1: Create a simple 2D dataset
data = np.array([
    [1, 2],
    [3, 5],
    [4, 2],
    [7, 8],
    [9, 1]
])

# Step 2: Build the KD-Tree
tree = KDTree(data, leaf_size=2)

# Step 3: Query the tree for nearest neighbors
query_point = np.array([[3, 3]])  # the point we want to find neighbors for
k = 2  # number of nearest neighbors

# Step 4: Find K nearest neighbors
dist, ind = tree.query(query_point, k=k)

print(f"Query Point: {query_point}")
print(f"{k} nearest neighbors (indices): {ind}")
print(f"{k} nearest neighbors (points): {data[ind]}")
print(f"Distances: {dist}")
