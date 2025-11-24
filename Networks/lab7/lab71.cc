#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab7Q1");

int main(int argc, char *argv[])
{
    Time::SetResolution(Time::NS);

    // Create 4 nodes: n0, n1, n2, n3
    NodeContainer nodes;
    nodes.Create(4);
    Ptr<Node> n0 = nodes.Get(0);
    Ptr<Node> n1 = nodes.Get(1);
    Ptr<Node> n2 = nodes.Get(2);
    Ptr<Node> n3 = nodes.Get(3);

    // Point-to-point channels
    // n0 <-> n2 : 2 Mbps, 10 ms
    PointToPointHelper p0p2;
    p0p2.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    p0p2.SetChannelAttribute("Delay", StringValue("10ms"));
    p0p2.SetQueue("ns3::DropTailQueue<Packet>",
                  "MaxSize", StringValue("100p"));

    // n1 <-> n2 : 2 Mbps, 10 ms
    PointToPointHelper p1p2;
    p1p2.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    p1p2.SetChannelAttribute("Delay", StringValue("10ms"));
    p1p2.SetQueue("ns3::DropTailQueue<Packet>",
                  "MaxSize", StringValue("100p"));

    // n2 <-> n3 : 1.7 Mbps, 20 ms
    PointToPointHelper p2p3;
    p2p3.SetDeviceAttribute("DataRate", StringValue("1.7Mbps"));
    p2p3.SetChannelAttribute("Delay", StringValue("20ms"));
    p2p3.SetQueue("ns3::DropTailQueue<Packet>",
                  "MaxSize", StringValue("100p"));

    NetDeviceContainer d0d2 = p0p2.Install(n0, n2);
    NetDeviceContainer d1d2 = p1p2.Install(n1, n2);
    NetDeviceContainer d2d3 = p2p3.Install(n2, n3);

    // Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;

    ipv4.SetBase("10.0.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i0i2 = ipv4.Assign(d0d2);

    ipv4.SetBase("10.0.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i2 = ipv4.Assign(d1d2);

    ipv4.SetBase("10.0.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i2i3 = ipv4.Assign(d2d3);

    Ipv4Address n3Addr = i2i3.GetAddress(1); // address of n3

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t cbrPort = 8000;
    uint16_t ftpPort = 21;

    // ------------------ CBR: n1 -> n3 (UDP) ------------------
    {
        // Receiver on n3
        Address sinkAddress(InetSocketAddress(n3Addr, cbrPort));
        PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", sinkAddress);
        ApplicationContainer sinkApps = sinkHelper.Install(n3);
        sinkApps.Start(Seconds(0.0));
        sinkApps.Stop(Seconds(5.0));

        // CBR sender on n1
        OnOffHelper onoff("ns3::UdpSocketFactory", sinkAddress);
        onoff.SetAttribute("DataRate", StringValue("1Mbps"));         // 1 Mbps
        onoff.SetAttribute("PacketSize", UintegerValue(1024));        // 1 kB
        onoff.SetAttribute("OnTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        ApplicationContainer cbrApp = onoff.Install(n1);
        cbrApp.Start(Seconds(0.1));   // start at 0.1 s
        cbrApp.Stop(Seconds(4.5));    // stop at 4.5 s
    }

    // ------------------ FTP: n0 -> n3 (TCP) ------------------
    {
        // Receiver on n3
        Address sinkAddress(InetSocketAddress(n3Addr, ftpPort));
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
        ApplicationContainer sinkApps = sinkHelper.Install(n3);
        sinkApps.Start(Seconds(0.0));
        sinkApps.Stop(Seconds(5.0));

        // FTP-like sender on n0 (BulkSend)
        BulkSendHelper bulk("ns3::TcpSocketFactory", sinkAddress);
        bulk.SetAttribute("MaxBytes", UintegerValue(0)); // send as much as possible
        ApplicationContainer ftpApp = bulk.Install(n0);
        ftpApp.Start(Seconds(1.0));   // start at 1 s
        ftpApp.Stop(Seconds(4.0));    // stop at 4 s
    }

    // ------------------ Flow Monitor ------------------
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(5.0));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;

    std::cout << "Per-flow statistics (only flows to n3):\n";

    for (const auto &it : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it.first);
        const FlowMonitor::FlowStats &st = it.second;

        if (t.destinationAddress != n3Addr)
        {
            continue;
        }

        std::string proto = (t.protocol == 6) ? "TCP" : (t.protocol == 17) ? "UDP" : "OTHER";

        std::cout << "Flow " << it.first << " (" << proto << ") "
                  << t.sourceAddress << " -> " << t.destinationAddress << "\n";
        std::cout << "  Tx Packets: " << st.txPackets
                  << "  Rx Packets: " << st.rxPackets << "\n\n";

        totalTxPackets += st.txPackets;
        totalRxPackets += st.rxPackets;
    }

    std::cout << "=====================================\n";
    std::cout << "Total transmitted packets to n3: " << totalTxPackets << "\n";
    std::cout << "Total received packets at n3   : " << totalRxPackets << "\n";
    std::cout << "=====================================\n";

    Simulator::Destroy();
    return 0;
}
