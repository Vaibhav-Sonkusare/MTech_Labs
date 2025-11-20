import numpy as np
import pandas as pd

df = pd.read_csv("_q1_dataset.csv")

print(df)

data = df[['Feature1', 'Feature2']].to_numpy()

sdata = (data - data.mean(axis=0)) / data.std(axis=0, ddof=1)
print("Standardized Dataset:")
print(sdata)

# Computing co-variance Matrix
cov_sdata = np.cov(sdata.T)
print("Covariance Matrix:")
print(cov_sdata)

# Calculating Eigen Values
e_value, e_vector = np.linalg.eig(cov_sdata)
# sorting eigen values and corrosponding eigen vectors
sorted_indices = np.argsort(e_value)[::-1]
e_value = e_value[sorted_indices]
e_vector = e_vector[:, sorted_indices]

print("Sorted Eigen Values are:")
print(e_value)
print("Corrosponding Eigen Vectors are:")
print(e_vector)

pc = [e_vector[:, i] for i in range(len(e_value))]
print("Principle components are:")
for i in range(len(pc)):
    print(f"PC{i + 1}: {pc[i]}, Variance: {e_value[i] / np.sum(e_value)}")

# Cumilative Variance
print("Cumulative Variance: ", np.cumsum(e_value / np.sum(e_value)))


# Projecting data to PC[0]
ndata = sdata.dot(pc[0].reshape(-1, 1))
print("Data Projected to PC1")
print(ndata)
