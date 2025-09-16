# Binary Classification - Fraud Detection (Synthetic Small Data)
"""
Dataset (Input → Output)
 Input Features: ["Amount", "Transaction_Time", "Location_Code"]
 Output: Fraud (1) or Not Fraud (0)
Amount Transaction_Time Location_Code Fraud
5000 23 3 1
120 14 1 0
300 19 2 0
7000 2 4 1
450 11 1 0
9000 22 3 1
100 10 1 0
6500 3 4 1
250 9 2 0
7200 1 3 1accuracy
Expected Output: Evaluation report: A classifier that correctly labels Fraud (1) vs
Not Fraud (0).
"""

import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.svm import SVC
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score
import matplotlib as plt

df = pd.read_csv("_q1_dataset.csv")
df = df.dropna()

df.loc[:, 'Amount'] = df.loc[:, 'Amount'] / 10
print(df)

x = df.iloc[:, :-1]
y = df.iloc[:, -1]
x_train, x_test, y_train, y_test = train_test_split(x, y, test_size=0.3)

model = SVC()
model.fit(X=x_train, y=y_train)
predicted = model.predict(X=x_test)

accuracy = accuracy_score(y_true=y_test, y_pred=predicted)
precision = precision_score(y_true=y_test, y_pred=predicted)
recall = recall_score(y_true=y_test, y_pred=predicted)
f1 = f1_score(y_true=y_test, y_pred=predicted)

print("Accuracy: ", accuracy)
print("precision: ", precision)
print("recall: ", recall)
print("f1: ", f1)
