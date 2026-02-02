import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn import metrics

# Data import and split
data = pd.read_csv("Naive-Bayes-Classification-Data.csv")
print(data)
X = data.drop('diabetes', axis=1)
y = data['diabetes']
# print(X)
# print(y)

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.3, random_state=40)

# K Means Classification
from sklearn.cluster import KMeans

kmeans_model = KMeans(n_clusters=2, random_state=0).fit(X_train)