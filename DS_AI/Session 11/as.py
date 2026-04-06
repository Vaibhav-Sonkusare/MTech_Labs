from pyspark.sql import SparkSession
from pyspark.sql.functions import col, split, upper, array_contains, sqrt

# Initialize Spark Session
spark = SparkSession.builder.appName("AssignmentPartB").getOrCreate()

# 1. Load Dataset (Assuming a CSV file exists)
df_raw = spark.read.csv("data.csv", header=True, inferSchema=True)

# Show initial data
print("Initial data input (data.csv):")
df_raw.show()

### 2. Data Pre-processing
# a. Handle Missing Values (Drop rows where Product is Null)
df_cleaned = df_raw.dropna(subset=["score"])

# b. Fill missing numeric values with the average or a constant
df_filled = df_cleaned.fillna({"skills": "Unknown"})

# c. Filtering Data
df_filtered = df_filled.filter(col("score") > 80)

# d. Final Transformation (Group by and Count)
# df_final = df_filtered.groupBy("Category").count()

print("Final Pre-processed Data:")
df_filtered.show()
# df_final.show()

### 1. Mathematical Operations

# 1. Math Manipulation (Calculate Square Root of Age)
df_filtered = df_filtered.withColumn("Age_Sqrt", sqrt(col("Age")))

# 2. String Manipulation (Uppercase Names)
df_filtered = df_filtered.withColumn("Name_Upper", upper(col("Name")))

# 3. Array Manipulation (Convert Skills string to Array and Check for 'Java')
df_filtered = df_filtered.withColumn("Skills_Array", split(col("Skills"), ","))
df_filtered = df_filtered.withColumn("Has_Java", array_contains(col("Skills_Array"), "Java"))

# Math, array and string manipulation
print("Math, Array and String Manipulation")
df_filtered.show()

# Stop the session
spark.stop()
