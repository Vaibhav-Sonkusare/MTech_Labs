import numpy as np
import pandas as pd

X = np.array([
    [1.0, 2.0],
    [1.5, 1.8],
    [5.0, 8.0],
    [6.0, 8.5],
    [1.2, 0.9],
    [6.5, 9.0]
])

n, d = X.shape
k = 2  # number of clusters

mu = np.array([[1.0, 2.0], [5.0, 8.0]])      # initial means
sigma = [np.eye(d) for _ in range(k)]        # identity covariance
pi = np.array([0.5, 0.5])                    # equal mixing coefficients

def gaussian_pdf(x, mean, cov):
    """Multivariate Gaussian PDF"""
    det = np.linalg.det(cov)
    inv = np.linalg.inv(cov)
    norm_const = 1.0 / (np.sqrt((2 * np.pi)**d * det))
    diff = x - mean
    return norm_const * np.exp(-0.5 * diff.T @ inv @ diff)

for iteration in range(2):
    # e step
    gamma = np.zeros((n, k))  # responsibilities

    for i in range(n):
        for j in range(k):
            gamma[i, j] = pi[j] * gaussian_pdf(X[i], mu[j], sigma[j])
        gamma[i, :] /= np.sum(gamma[i, :])  # normalize

    # m step
    N_k = np.sum(gamma, axis=0)

    for j in range(k):
        # Update means
        mu[j] = np.sum(gamma[:, j].reshape(-1, 1) * X, axis=0) / N_k[j]

        # Update covariances
        diff = X - mu[j]
        sigma[j] = (gamma[:, j].reshape(-1, 1) * diff).T @ diff / N_k[j]

        # Update mixing coefficients
        pi[j] = N_k[j] / n

    print(f"\nIteration {iteration + 1}")
    print("Responsibilities :\n", gamma)
    print("Means:\n", mu)
    print("Mixing coefficients:", pi)
    print("Covariances:")
    for j in range(k):
        print(f"Cluster {j+1}:\n", sigma[j])

assignments = np.argmax(gamma, axis=1)
print("\nFinal Cluster Assignments:", assignments)
