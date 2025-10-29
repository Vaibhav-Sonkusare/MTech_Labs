import numpy as np
import matplotlib.pyplot as plt
from sklearn.cluster import KMeans

# Step 1: Create some sample 2D data
X = np.array([
    [2, 3], [3, 3], [3, 4],
    [8, 7], [7, 8], [9, 7],
    [5, 2], [6, 2], [5, 3]
])

# Step 2: Apply K-Means with 3 clusters
kmeans = KMeans(n_clusters=3, random_state=0)
kmeans.fit(X)

# Step 3: Get results
labels = kmeans.labels_        # cluster assigned to each point
centroids = kmeans.cluster_centers_

print("Cluster Labels:", labels)
print("Cluster Centers:\n", centroids)

# Step 4: Visualize the clusters
colors = ['red', 'green', 'blue']

for i in range(3):
    cluster_points = X[labels == i]
    plt.scatter(cluster_points[:, 0], cluster_points[:, 1], c=colors[i], label=f'Cluster {i+1}')

# Plot centroids
plt.scatter(centroids[:, 0], centroids[:, 1], c='black', marker='x', s=100, label='Centroids')

plt.title("K-Means Clustering Example")
plt.xlabel("X-axis")
plt.ylabel("Y-axis")
plt.legend()
plt.show()
