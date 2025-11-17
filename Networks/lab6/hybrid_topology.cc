// ========================= hybrid_topology.cc =========================
// Hybrid: Star + Chain with proper QueueDisc handling (ns-3.38 safe)

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

#include <sstream>
#include <fstream>

using namespace ns3;

static Ipv4Address GetNodeIp (Ptr<Node> node)
{
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    for (uint32_t i = 0; i < ipv4->GetNInterfaces(); i++)
        for (uint32_t j = 0; j < ipv4->GetNAddresses(i); j++)
        {
            Ipv4InterfaceAddress addr = ipv4->GetAddress(i, j);
            if (addr.GetLocal() != Ipv4Address::GetLoopback())
                return addr.GetLocal();
        }
    return Ipv4Address::GetAny();
}

int main (int argc, char *argv[])
{
    uint32_t N = 10;
    uint32_t chainLen = 4;
    double simTime = 100.0;

    std::string csvPrefix = "results_hybrid";

    CommandLine cmd;
    cmd.AddValue("N", "Number of nodes", N);
    cmd.AddValue("chainLen", "Length of chain", chainLen);
    cmd.Parse(argc, argv);

    if (N < 3) { std::cout << "Need >=3 nodes\n"; return 1; }
    if (chainLen >= N) chainLen = N/2;

    NodeContainer nodes;
    nodes.Create(N);

    InternetStackHelper stack;
    stack.Install(nodes);

    // Configure P2P
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    // QueueDisc helper (drop-tail)
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::PfifoFastQueueDisc", "MaxSize", StringValue("10p"));

    Ipv4AddressHelper address;
    char subnet[20];

    // -------------------------------
    // Build STAR (hub=0, leaves=1..N-1)
    // -------------------------------
    for (uint32_t i = 1; i < N; ++i)
    {
        NodeContainer pair(nodes.Get(0), nodes.Get(i));
        NetDeviceContainer dev = p2p.Install(pair);

        // Install QueueDisc ONCE immediately
        tch.Install(dev);

        sprintf(subnet, "10.1.%d.0", i);
        address.SetBase(subnet, "255.255.255.0");
        address.Assign(dev);
    }

    // -------------------------------
    // Build CHAIN on last chainLen nodes
    // -------------------------------
    uint32_t start = (N > chainLen) ? (N - chainLen) : 1;

    for (uint32_t i = start; i < N - 1; ++i)
    {
        NodeContainer pair(nodes.Get(i), nodes.Get(i + 1));
        NetDeviceContainer dev = p2p.Install(pair);

        // Install QueueDisc ONCE immediately
        tch.Install(dev);

        sprintf(subnet, "10.2.%d.0", i);
        address.SetBase(subnet, "255.255.255.0");
        address.Assign(dev);
    }

    // -------------------------------
    // APPLICATIONS
    // -------------------------------
    uint16_t port = 60000;
    ApplicationContainer sinks, sources;

    for (uint32_t i = 1; i < N; i += 2)
    {
        uint32_t dest = i + 1;
        if (dest >= N) break;

        Ipv4Address destIp = GetNodeIp(nodes.Get(dest));
        if (destIp == Ipv4Address::GetAny()) continue;

        PacketSinkHelper sink("ns3::UdpSocketFactory",
                               InetSocketAddress(Ipv4Address::GetAny(), port));
        sinks.Add(sink.Install(nodes.Get(dest)));

        OnOffHelper onoff("ns3::UdpSocketFactory",
                          InetSocketAddress(destIp, port));
        onoff.SetConstantRate(DataRate("7Mbps"), 1024);  // overloaded → drops
        sources.Add(onoff.Install(nodes.Get(i)));
    }

    sinks.Start(Seconds(0.1));
    sinks.Stop(Seconds(simTime+1));

    sources.Start(Seconds(1.0));
    sources.Stop(Seconds(simTime));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // -------------------------------
    // FLOW MONITOR
    // -------------------------------
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> fm = fmh.InstallAll();

    Simulator::Stop(Seconds(simTime+1));
    Simulator::Run();

    uint64_t totalRx = 0, totalTx = 0, totalRxPk = 0;

    for (auto &kv : fm->GetFlowStats())
    {
        totalRx += kv.second.rxBytes;
        totalTx += kv.second.txPackets;
        totalRxPk += kv.second.rxPackets;
    }

    double throughputMbps = (totalRx * 8.0) / (simTime * 1e6);
    uint64_t packetLoss = totalTx - totalRxPk;

    // CSV output
    std::ostringstream fname;
    fname << csvPrefix << "_" << N << ".csv";
    std::ofstream out(fname.str());
    out << "Nodes,Throughput_Mbps,PacketLoss\n";
    out << N << "," << throughputMbps << "," << packetLoss << "\n";
    out.close();

    std::cout << "HYBRID TOPOLOGY - N=" << N
              << "  Throughput(Mbps): " << throughputMbps
              << "  PacketLoss: " << packetLoss << "\n";

    Simulator::Destroy();
    return 0;
}
