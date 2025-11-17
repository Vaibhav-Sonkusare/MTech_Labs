import pandas as pd
import matplotlib.pyplot as plt

Ns = [5, 10, 15, 20, 25]
throughput = []
packet_loss = []

for N in Ns:
    df = pd.read_csv(f"/home/mtech/ns-allinone-3.38/ns-3.38/results_bus_{N}.csv")
    throughput.append(df['Throughput_Mbps'][0])
    packet_loss.append(df['PacketLoss'][0])

# Graph 1: Throughput vs Number of Nodes
plt.figure()
plt.plot(Ns, throughput, marker='o')
plt.xlabel("Number of Nodes")
plt.ylabel("Throughput (Mbps)")
plt.title("Throughput vs Number of Nodes (Bus Topology)")
plt.grid(True)
plt.show()
plt.savefig("bus_Throughput_vs_Number_of_Nodes.png")

# Graph 2: Packet Loss vs Number of Nodes
plt.figure()
plt.plot(Ns, packet_loss, marker='o')
plt.xlabel("Number of Nodes")
plt.ylabel("Packet Loss")
plt.title("Packet Loss vs Number of Nodes (Bus Topology)")
plt.grid(True)
plt.show()
plt.savefig("bus_Packet_Loss_vs_Number_of_Nodes.png")
