import numpy as np
import pandas as pd
from sklearn.cluster import KMeans
import math

df = pd.read_csv("_q1_dataset.csv")
df = df.dropna()
data = np.array(df).T

print("--------Q1.1--------")
print("Original Dataset:")
print(data)

# Normalizing features between 0 and 1
feature1_min = min(data[1])
feature1_max = max(data[1])
for x in range(len(data[1])):
    data[1][x] -= feature1_min
    data[1][x] /= feature1_max - feature1_min
feature2_min = min(data[2])
feature2_max = max(data[2])
for x in range(len(data[2])):
    data[2][x] -= feature2_min
    data[2][x] /= feature2_max - feature2_min

print("\nDataset after normalizing features:")
print(data)


print("--------Q1.2, Q1.3--------")

def euclidean_distance(a, b):
    return math.sqrt(math.pow((a[0] - b[0]), 2) + math.pow((a[0] - b[0]), 2))

def get_centroid_of_class(data: np.ndarray, set: str):
    cc = [0.0, 0.0]     # Cluster Center
    count = 0
    for x in data:
        if x[3] != set:
            continue
        cc[0] += x[1]
        cc[1] += x[2]
        count += 1
    cc[0] /= count
    cc[1] /= count
    return cc

centroids = []
class_labels = []
for x in data[3]:
    if x not in class_labels:
        centroids.append(get_centroid_of_class(data.T, x))
        class_labels.append(x)

print("Classifying new sample: Feature1 = 2.2, Feature2 = 2.7")
new_feature1 = 2.2
new_feature2 = 2.7
nf1 = (new_feature1 - feature1_min) / (feature1_max - feature1_min)  # normalized feature 1
nf2 = (new_feature2 - feature2_min) / (feature2_max - feature2_min)  # normalized feature 1

k = 3
min_distance = math.inf
nf_class = class_labels[0]
# for x in range(len(class_labels)):
#     distance = euclidean_distance((nf1, nf2), centroids[x])
#     print("Distance between new feature and class", class_labels[x], "=", distance)
#     if min_distance > distance:
#         # print("class changed from", nf_class, "to", class_labels[x])
#         nf_class = class_labels[x]
#     # else:
#         # print("class not changed from", nf_class, "to", class_labels[x])
#     min_distance = min(min_distance, distance)
data = data.T
min_distance = [[math.inf, class_labels[0]]]*k
for x in range(len(data)):
    distance = euclidean_distance((nf1, nf2), (data[x][1], data[x][2]))
    print("Distance between new feature and point", x, "=", distance)
    print("array", min_distance)
    if min_distance[0][0] > distance:
        min_distance[0][0] = min(min_distance[0][0], distance)
        min_distance[0][1] = data[x][3]
    print("arr", min_distance)
    min_distance.sort(key=lambda x: x[0], reverse=True)
    print("new arr", min_distance)

print("closest classes are:\n", min_distance)
print("Final Assigned class: ", nf_class)

print("--------Q1.4--------")
print("As there are three class given in our dataset, the value of\n" \
"\tk = 3 will give highest accuracy for the dataset")