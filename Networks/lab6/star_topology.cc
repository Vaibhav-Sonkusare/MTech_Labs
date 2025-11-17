#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("StarTopology");

int main(int argc, char *argv[])
{
    uint32_t N = 5;
    std::string csvPrefix = "results_star";

    CommandLine cmd;
    cmd.AddValue("N", "Number of leaf nodes", N);
    cmd.Parse(argc, argv);

    // Create hub + leaves
    NodeContainer hub;
    hub.Create(1);

    NodeContainer leaves;
    leaves.Create(N);

    std::vector<NodeContainer> linkNodes;
    for (uint32_t i = 0; i < N; ++i)
        linkNodes.push_back(NodeContainer(hub.Get(0), leaves.Get(i)));

    // P2P configuration (creates congestion)
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    std::vector<NetDeviceContainer> devs;
    for (uint32_t i = 0; i < N; ++i)
        devs.push_back(p2p.Install(linkNodes[i]));

    InternetStackHelper stack;
    stack.Install(hub);
    stack.Install(leaves);

    // Apply queue disc (drop-tail)
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::PfifoFastQueueDisc", "MaxSize",
                         StringValue("10p"));

    for (uint32_t i = 0; i < N; ++i)
        tch.Install(devs[i]);

    // IP assignment
    Ipv4AddressHelper address;
    std::vector<Ipv4InterfaceContainer> interfaces;

    for (uint32_t i = 0; i < N; ++i)
    {
        std::stringstream ss;
        ss << "10.1." << (i + 1) << ".0";
        address.SetBase(ss.str().c_str(), "255.255.255.0");
        interfaces.push_back(address.Assign(devs[i]));
    }

    uint16_t port = 9000;

    // ⭐ Only ONE sink on the hub → avoids bind conflicts
    PacketSinkHelper sink("ns3::UdpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sink.Install(hub.Get(0));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(10.0));

    // Each leaf → hub (on its interface IP)
    ApplicationContainer sourceApps;

    for (uint32_t i = 0; i < N; ++i)
    {
        Ipv4Address hubIf = interfaces[i].GetAddress(0);

        OnOffHelper onoff("ns3::UdpSocketFactory",
                          InetSocketAddress(hubIf, port));

        onoff.SetConstantRate(DataRate("7Mbps"), 1024); // overload
        sourceApps.Add(onoff.Install(leaves.Get(i)));
    }

    sourceApps.Start(Seconds(2.0));
    sourceApps.Stop(Seconds(9.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // FlowMonitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    uint64_t totalRxBytes = 0;
    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;

    monitor->CheckForLostPackets();
    auto stats = monitor->GetFlowStats();

    for (auto &flow : stats)
    {
        totalRxBytes += flow.second.rxBytes;
        totalTxPackets += flow.second.txPackets;
        totalRxPackets += flow.second.rxPackets;
    }

    double throughputMbps =
        (totalRxBytes * 8.0) / (7.0 * 1e6);

    uint64_t packetLoss = totalTxPackets - totalRxPackets;

    // Write CSV
    std::ostringstream fname;
    fname << csvPrefix << "_" << N << ".csv";
    std::ofstream out(fname.str());
    out << "Nodes,Throughput_Mbps,PacketLoss\n";
    out << N << "," << throughputMbps << "," << packetLoss << "\n";
    out.close();

    std::cout << "STAR TOPOLOGY - N=" << N
              << "  Throughput(Mbps): " << throughputMbps
              << "  PacketLoss: " << packetLoss << std::endl;

    Simulator::Destroy();
    return 0;
}
