# Simple Linear Regression

import pandas as pd
import numpy as np
from sklearn.linear_model import LinearRegression
from sklearn.metrics import accuracy_score
import matplotlib.pyplot as plt

class MyLinearRegression:
    def __init__(self):
        self.parameters = {}
    
    def load_data(self, filename: str) -> None:
        data = pd.read_csv(filename)
        data = data.dropna()
        
        self.parameters['train_input'] = np.array(data.iloc[0:500, 0]).reshape(500, 1)
        self.parameters['train_output'] = np.array(data.iloc[0:500, 1]).reshape(500, 1)

        self.parameters['test_input'] = np.array(data.iloc[500:699, 0]).reshape(199, 1)
        self.parameters['test_output'] = np.array(data.iloc[500:699, 1]).reshape(199, 1)
    
    def fun(self) -> None:
        model = LinearRegression()
        model.fit(X=self.parameters['train_input'], y=self.parameters['train_output'])
        predicted = model.predict(self.parameters['test_input'])

        accuracy = accuracy_score(y_true=self.parameters['test_output'], y_pred=predicted)
        print(accuracy)

        # graph = plt.

def main():
    LR = MyLinearRegression()
    LR.load_data('data_for_lr.csv')
    # print(LR.parameters)
    LR.fun()

if __name__ == "__main__":
    main()