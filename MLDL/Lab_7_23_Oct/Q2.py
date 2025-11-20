import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("_q2_dataset.csv")

data = df[['Purchase Frequency', 'Avg_Spending', 'Discount Used', 'Visits_Per_Month']].to_numpy()

sdata = (data - data.mean(axis=0)) / data.std(axis=0, ddof=1)
print("Standardized Dataset:")
print(sdata)

cov_sdata = np.cov(sdata.T)
e_value, e_vector = np.linalg.eig(cov_sdata)
sorted_indices = np.argsort(e_value)[::-1]
e_value = e_value[sorted_indices]
e_vector = e_vector[:, sorted_indices]
print("Sorted Eigen Values:")
print(e_value)
print("Corresponding Eigen Vectors:")
print(e_vector)

pc = [e_vector[:, i] for i in range(len(e_value))]
for i in range(len(pc)):
    print(f"PC{i+1}: {pc[i]}, Variance % = {(e_value[i] * 100) / np.sum(e_value)}%")

# ====== 🧭 NEW SECTION: Identify which features drive PC1 and PC2 ======
feature_names = ['Purchase Frequency', 'Avg_Spending', 'Discount Used', 'Visits_Per_Month']
loadings = pd.DataFrame(e_vector[:, :2], columns=['PC1', 'PC2'], index=feature_names)

print("\nFeature Loadings for First Two Principal Components:")
print(loadings)

# Show top contributing features for PC1 and PC2
for i in range(2):
    pc_name = f'PC{i+1}'
    sorted_feats = loadings[pc_name].abs().sort_values(ascending=False)
    top_feat = sorted_feats.index[0]
    print(f"\nTop feature driving {pc_name}: {top_feat} "
          f"(loading = {loadings.loc[top_feat, pc_name]:.3f})")

# ================================================================

# Projecting data on PC1, PC2
ndata = np.dot(sdata, e_vector[:, :2])
# print("1:", ndata)

spending_level = df['Avg_Spending']
# print("2:", spending_level)

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
plt.savefig("_Q2_fig.jpeg")
print("Plot saved in _Q2_fig.jpeg")
