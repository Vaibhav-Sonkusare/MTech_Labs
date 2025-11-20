import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("_q2_dataset.csv")

data = df[['Purchase Frequency', 'Avg_Spending', 'Discount Used', 'Visits_Per_Month']].to_numpy()

sdata = (data - data.mean(axis=0)) / data.std(axis=0, ddof=1)
print("Standerdised Dataset:")
print(sdata)

cov_sdata = np.cov(sdata.T)
e_value, e_vector = np.linalg.eig(cov_sdata)
sorted_indices = np.argsort(e_value)[::-1]
e_value = e_value[sorted_indices]
e_vector = e_vector[:, sorted_indices]
print("Sorted  Eigen Values:")
print(e_value)
print("Corrosponding Eigen Vectors:")
print(e_vector)

pc = [e_vector[:, i] for i in range(len(e_value))]
for i in range(len(pc)):
    print(f"PC{i+1}: {pc[i]}, Variance % = {(e_value[i] * 100) / np.sum(e_value)}%")

# Projecting data on PC1, PC2
# ndata = np.dot(sdata, pc[:2])
ndata = np.dot(sdata, e_vector[:, :2])
spending_level = df['Avg_Spending']

plt.figure(figsize=(8,6))
plt.scatter(ndata[:, 0], ndata[:, 1],
            c=spending_level, cmap='viridis', s=100, edgecolors='k')

for i, cid in enumerate(df['Customer ID']):
    plt.text(ndata[i, 0] + 0.05, ndata[i, 1], f'Cust {cid}', fontsize=9)

plt.xlabel('Principal Component 1')
plt.ylabel('Principal Component 2')
plt.title('PCA on Customer Purchase Behavior')
plt.colorbar(label='Avg Spending')
plt.grid(True)
# plt.show()
plt.savefig("_q2_fig.jpeg")

feature_names = ['Purchase Frequency', 'Avg_Spending', 'Discount Used', 'Visits_Per_Month']
loadings = pd.DataFrame(e_vector[:, :2], columns=['PC1', 'PC2'], index=feature_names)

print("\nFeature Loadings for First Two Principal Components:")
print(loadings)