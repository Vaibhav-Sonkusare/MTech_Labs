from pymongo import MongoClient
import pandas as pd
from sklearn.preprocessing import LabelEncoder

# Connect to MongoDB
client = MongoClient("mongodb://localhost:27017/")
db = client["company"]
collection = db["employees"]

# Fetch data
data = list(collection.find({}, {"_id": 0}))
df = pd.DataFrame(data)

print("Original Data:")
print(df)

# Encode department
le = LabelEncoder()
df["department_encoded"] = le.fit_transform(df["dept"])

print("\nEncoded Data:")
print(df)

from sklearn.linear_model import LinearRegression

# Features and target
X = df[["department_encoded"]]
y = df["age"]

# Train model
model = LinearRegression()
model.fit(X, y)

# Predict example
for i in range(1,4):
    predicted_age = model.predict([[i]])
    print("\nPredicted Age for ", i, predicted_age)