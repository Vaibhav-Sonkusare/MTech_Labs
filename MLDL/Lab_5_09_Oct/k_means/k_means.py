import random
import math

# Step 1: Sample 2D data points
points = [
    (2, 3), (3, 3), (3, 4),
    (8, 7), (7, 8), (9, 7),
    (5, 2), (6, 2), (5, 3)
]

# Step 2: Number of clusters
k = 3

# Step 3: Initialize centroids randomly from the data points
centroids = random.sample(points, k)

def distance(p1, p2):
    return math.sqrt((p1[0] - p2[0])**2 + (p1[1] - p2[1])**2)

# Step 4: K-Means iterative process
for iteration in range(10):  # limit to 10 iterations
    print(f"\nIteration {iteration + 1}")
    
    # Assign points to nearest centroid
    clusters = {i: [] for i in range(k)}
    for point in points:
        nearest_centroid_index = min(range(k), key=lambda i: distance(point, centroids[i]))
        clusters[nearest_centroid_index].append(point)
    
    # Display cluster assignment
    for i in range(k):
        print(f"Cluster {i + 1}: {clusters[i]}")
    
    # Recalculate centroids
    new_centroids = []
    for i in range(k):
        if clusters[i]:  # avoid division by zero
            x_mean = sum(p[0] for p in clusters[i]) / len(clusters[i])
            y_mean = sum(p[1] for p in clusters[i]) / len(clusters[i])
            new_centroids.append((x_mean, y_mean))
        else:
            new_centroids.append(centroids[i])  # keep the same if empty
    
    # Check for convergence (if centroids didn’t change)
    if all(distance(centroids[i], new_centroids[i]) < 1e-4 for i in range(k)):
        print("\nConverged!")
        break
    
    centroids = new_centroids

print("\nFinal Centroids:")
for i, c in enumerate(centroids):
    print(f"Centroid {i + 1}: {c}")
