"""Q1. Entropy & Information Gain Calculation
Given the dataset:
WeatherTemperaturePlay (Target)
Sunny
SunnyHot
HotNo
No
Overcast
RainHot
MildYes
Yes
Rain
Rain
Overcast
SunnyCool
Cool
Cool
MildYes
No
Yes
No
Sunny
RainCool
MildYes
Yes
1. Calculate the Entropy of the target variable Play.
2. Calculate the Information Gain (IG) when splitting on Weather.
3. Decide whether Weather is a good splitting attribute.
Expected Output:
Entropy(Play) ≈ ?
IG(Weather) ≈ ?
Weather is a useful attribute, but not the best (later compare with Temperature).
"""

import numpy as np
import pandas as pd
import math


df = pd.read_csv("_q1_dataset.csv")
df = df.dropna()

# Data Cleaning
df.loc[df['Weather'] == 'Sunny', ['Weather']] = 0
df.loc[df['Weather'] == 'Rain', ['Weather']] = 1
df.loc[df['Weather'] == 'Overcast', ['Weather']] = 2

df.loc[df['Temperature'] == 'Hot', ['Temperature']] = 0
df.loc[df['Temperature'] == 'Mild', ['Temperature']] = 1
df.loc[df['Temperature'] == 'Cool', ['Temperature']] = 2

df.loc[df['Play(Target)'] == 'Yes', ['Play(Target)']] = 1
df.loc[df['Play(Target)'] == 'No', ['Play(Target)']] = 0

data = np.array(df)

goalatrs = [0, 1]




def main():
    pass


if __name__ == "__main__":
    main()