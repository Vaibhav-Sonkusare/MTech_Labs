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

def calculateTargetEntropy() -> float:
    N = len(data)
    atrs = {}
    for entry in data:
        if entry[2] not in atrs:
            atrs[entry[2]] = 1
        else:
            atrs[entry[2]] += 1

    ans = 0

    for atr, _ in enumerate(atrs):
        _n = atrs[atr]
        ans -= (_n/N) * math.log2((_n / N))
    
    return ans

def calculateEntropy(target: int) -> float:
    N = len(data)

    atrs = {}
    for entry in data:
        if entry[target] not in atrs:
            atrs[entry[target]] = {}
            for x in goalatrs:
                atrs[entry[target]][x] = 0
        for x in goalatrs:
            if x == entry[2]:
                atrs[entry[target]][x] += 1
                break
    
    ans = 0
    for atr, _ in enumerate(atrs):
        temp = 0
        _N = sum([atrs[atr][x] for x in goalatrs])
        for x in goalatrs:
            _n = atrs[atr][x]
            if _n != 0:
                temp -= (_n / _N) * math.log2((_n / _N))
                # print("_n = ", _n, "_N = ", _N, "log(", _n/_N, ") = ", math.log2((_n/_N)))
            # print("_n = ", _n, "temp = ", temp)
        temp *= _N / N
        ans += temp
        # print("ans = ", ans)

    return ans

def InformationGain(target: int) -> float:
    return calculateTargetEntropy() - calculateEntropy(target=target)


def main():
    print("Target(Play) Entropy", calculateTargetEntropy())
    colm_name = ["Weather","Temperature"]
    for i in range(len(colm_name)):
        print("Entropy of ", colm_name[i], "= ", calculateEntropy(i))
    for i in range(len(colm_name)):
        print("Information Gain of ", colm_name[i], "= ", InformationGain(i))


if __name__ == "__main__":
    main()