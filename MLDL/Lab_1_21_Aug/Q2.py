"""
Q2: Multi-class Classification – Disease Prediction (Synthetic
Small Data)
Dataset (Input → Output)
Input Features: ["Temperature", "Cough_Level", "Age"]
Output: Disease → {0: Healthy, 1: Flu, 2: Pneumonia}
TemperatureCough_LevelAge Disease
98.6025
0
101.2230
1
103.5340
2
99.1028
0
100.8235
1
102.7350
2
98.4022
0
101.5245
1
104.0355
2
99.0027
0
Expected Output: Evaluation report: A classifier that predicts
whether a person is Healthy (0), Flu (1), or Pneumonia (2).
"""

import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.svm import SVC
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score
import matplotlib as plt

df = pd.read_csv("_q2_dataset.csv")
df = df.dropna()

x = df.iloc[:, :-1]
y = df.iloc[:, -1]

x_train, x_test, y_train, y_test = train_test_split(x, y, test_size=0.3, stratify=y)
print(x_train, x_test, y_train, y_test, sep="\n")

model = SVC()
model.fit(X=x_train, y=y_train)
predicted = model.predict(X=x_test)

print(predicted)

accuracy = accuracy_score(y_true=y_test, y_pred=predicted)
precision = precision_score(y_true=y_test, y_pred=predicted, average='macro')
recall = recall_score(y_true=y_test, y_pred=predicted, average='macro')
f1 = f1_score(y_true=y_test, y_pred=predicted, average='macro')

print("Accuracy: ", accuracy)
print("precision: ", precision)
print("recall: ", recall)
print("f1: ", f1)
