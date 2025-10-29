import math

# Step 1: Define a simple 2D dataset
points = [
    (2, 3),
    (5, 4),
    (9, 6),
    (4, 7),
    (8, 1),
    (7, 2)
]

# Step 2: Define KD-Tree node structure
class Node:
    def __init__(self, point, left=None, right=None):
        self.point = point
        self.left = left
        self.right = right

# Step 3: Function to build KD-Tree recursively
def build_kdtree(points, depth=0):
    if not points:
        return None
    
    k = len(points[0])          # number of dimensions
    axis = depth % k            # alternate between axes
    
    # Sort points by the selected axis and choose median
    points.sort(key=lambda x: x[axis])
    median = len(points) // 2
    
    # Create node and construct subtrees
    return Node(
        point=points[median],
        left=build_kdtree(points[:median], depth + 1),
        right=build_kdtree(points[median + 1:], depth + 1)
    )

# Step 4: Distance function (Euclidean)
def distance_squared(p1, p2):
    return sum((a - b) ** 2 for a, b in zip(p1, p2))

# Step 5: KNN search (find nearest neighbor)
def nearest_neighbor(root, target, depth=0, best=None):
    if root is None:
        return best
    
    k = len(target)
    axis = depth % k
    
    # Initialize best
    if best is None or distance_squared(target, root.point) < distance_squared(target, best):
        best = root.point
    
    # Compare target with current node
    if target[axis] < root.point[axis]:
        next_branch, opposite_branch = root.left, root.right
    else:
        next_branch, opposite_branch = root.right, root.left
    
    # Search down the tree
    best = nearest_neighbor(next_branch, target, depth + 1, best)
    
    # Check whether we need to explore the opposite branch
    if (target[axis] - root.point[axis]) ** 2 < distance_squared(target, best):
        best = nearest_neighbor(opposite_branch, target, depth + 1, best)
    
    return best

# Step 6: Build the tree and test
root = build_kdtree(points)
target = (9, 2)
nearest = nearest_neighbor(root, target)

print(f"Target Point: {target}")
print(f"Nearest Neighbor: {nearest}")
