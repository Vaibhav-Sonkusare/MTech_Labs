#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Lab7Q2");

int main(int argc, char *argv[])
{
    Time::SetResolution(Time::NS);

    // Node counts to test
    std::vector<uint32_t> nodeCounts = {10, 15, 20, 25, 30, 40};

    CommandLine cmd;
    cmd.Parse(argc, argv);

    for (uint32_t N : nodeCounts)
    {
        std::cout << "\n\n\n\nN = " << N << "\n";

        // ---------- 1. Create nodes ----------
        NodeContainer nodes;
        nodes.Create(N);

        Ptr<Node> sink   = nodes.Get(0);
        Ptr<Node> router = nodes.Get(N - 1);

        // ---------- 2. Create links ----------
        PointToPointHelper p2pSrcRouter;
        p2pSrcRouter.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
        p2pSrcRouter.SetChannelAttribute("Delay", StringValue("2ms"));
        p2pSrcRouter.SetQueue("ns3::DropTailQueue<Packet>",
                              "MaxSize", StringValue("100p"));

        PointToPointHelper p2pRouterSink;
        p2pRouterSink.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
        p2pRouterSink.SetChannelAttribute("Delay", StringValue("10ms"));
        p2pRouterSink.SetQueue("ns3::DropTailQueue<Packet>",
                               "MaxSize", StringValue("100p"));

        InternetStackHelper stack;
        stack.Install(nodes);

        Ipv4AddressHelper ipv4;
        std::vector<Ipv4InterfaceContainer> srcIfs;

        // Connect each source (1..N-2) to router with its own subnet
        for (uint32_t i = 1; i < N - 1; ++i)
        {
            NodeContainer pair(nodes.Get(i), router);
            NetDeviceContainer devs = p2pSrcRouter.Install(pair);

            std::ostringstream subnet;
            subnet << "10.1." << i << ".0";
            ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
            Ipv4InterfaceContainer ifc = ipv4.Assign(devs);
            srcIfs.push_back(ifc);
        }

        // Router <-> sink link
        NodeContainer rs(router, sink);
        NetDeviceContainer rsDevs = p2pRouterSink.Install(rs);
        ipv4.SetBase("10.2.0.0", "255.255.255.0");
        Ipv4InterfaceContainer rsIf = ipv4.Assign(rsDevs);
        Ipv4Address sinkAddr = rsIf.GetAddress(1);

        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

        // ---------- 3. Install applications ----------
        ApplicationContainer allApps;

        double appStart = 0.5;
        double appStop  = 9.5;
        uint16_t basePort = 9000;

        for (uint32_t i = 1; i < N - 1; ++i)
        {
            bool useTcp = (i % 2 == 0);   // even index -> TCP, odd -> UDP
            uint16_t port = basePort + i;

            Address sinkAddress(InetSocketAddress(sinkAddr, port));

            if (useTcp)
            {
                // TCP sink on sink node
                PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
                ApplicationContainer sinkApp = sinkHelper.Install(sink);
                sinkApp.Start(Seconds(0.0));
                sinkApp.Stop(Seconds(10.0));
                allApps.Add(sinkApp);

                // TCP BulkSend on source i
                BulkSendHelper bulk("ns3::TcpSocketFactory", sinkAddress);
                bulk.SetAttribute("MaxBytes", UintegerValue(0)); // unlimited
                ApplicationContainer srcApp = bulk.Install(nodes.Get(i));
                srcApp.Start(Seconds(appStart));
                srcApp.Stop(Seconds(appStop));
                allApps.Add(srcApp);
            }
            else
            {
                // UDP sink
                PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", sinkAddress);
                ApplicationContainer sinkApp = sinkHelper.Install(sink);
                sinkApp.Start(Seconds(0.0));
                sinkApp.Stop(Seconds(10.0));
                allApps.Add(sinkApp);

                // UDP CBR OnOff on source i
                OnOffHelper onoff("ns3::UdpSocketFactory", sinkAddress);
                onoff.SetAttribute("DataRate", StringValue("1Mbps"));
                onoff.SetAttribute("PacketSize", UintegerValue(1024));
                onoff.SetAttribute("OnTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                onoff.SetAttribute("OffTime",
                                   StringValue("ns3::ConstantRandomVariable[Constant=0]"));

                ApplicationContainer srcApp = onoff.Install(nodes.Get(i));
                srcApp.Start(Seconds(appStart));
                srcApp.Stop(Seconds(appStop));
                allApps.Add(srcApp);
            }
        }

        // ---------- 4. Flow monitor ----------
        FlowMonitorHelper flowmonHelper;
        Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

        Simulator::Stop(Seconds(10.0));
        Simulator::Run();

        monitor->CheckForLostPackets();
        Ptr<Ipv4FlowClassifier> classifier =
            DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
        auto stats = monitor->GetFlowStats();

        // ---------- 5. Compute metrics ----------
        uint64_t tcpTxPkts = 0, tcpRxPkts = 0;
        uint64_t udpTxPkts = 0, udpRxPkts = 0;
        double tcpDelaySum = 0.0, udpDelaySum = 0.0;
        uint64_t tcpRxForDelay = 0, udpRxForDelay = 0;

        std::vector<double> throughputs;  // for Jain fairness

        for (const auto &it : stats)
        {
            Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it.first);
            const FlowMonitor::FlowStats &st = it.second;

            // Only consider flows that end at the sink
            if (t.destinationAddress != sinkAddr)
                continue;

            double start = st.timeFirstTxPacket.GetSeconds();
            double end   = st.timeLastRxPacket.GetSeconds();
            double duration = end - start;
            double thr = 0.0;
            if (duration > 0.0)
            {
                thr = (st.rxBytes * 8.0) / (duration * 1e6);  // Mbps
                throughputs.push_back(thr);
            }

            bool isTcp = (t.protocol == 6); // TCP=6, UDP=17

            if (isTcp)
            {
                tcpTxPkts += st.txPackets;
                tcpRxPkts += st.rxPackets;
                tcpDelaySum += st.delaySum.GetSeconds();
                tcpRxForDelay += st.rxPackets;
            }
            else // UDP
            {
                udpTxPkts += st.txPackets;
                udpRxPkts += st.rxPackets;
                udpDelaySum += st.delaySum.GetSeconds();
                udpRxForDelay += st.rxPackets;
            }
        }

        // Averages and fairness index
        double tcpAvgDelay = (tcpRxForDelay > 0) ? tcpDelaySum / tcpRxForDelay : 0.0;
        double udpAvgDelay = (udpRxForDelay > 0) ? udpDelaySum / udpRxForDelay : 0.0;

        uint64_t tcpLoss = (tcpTxPkts > tcpRxPkts) ? (tcpTxPkts - tcpRxPkts) : 0;
        uint64_t udpLoss = (udpTxPkts > udpRxPkts) ? (udpTxPkts - udpRxPkts) : 0;

        double sum = 0.0, sumSq = 0.0;
        for (double x : throughputs)
        {
            sum += x;
            sumSq += x * x;
        }
        double fairness = 0.0;
        if (!throughputs.empty())
        {
            fairness = (sum * sum) / (throughputs.size() * sumSq);
        }

        // ---------- 6. Print results ----------
        std::cout << "TCP:\n";
        std::cout << "  Tx Packets   = " << tcpTxPkts << "\n";
        std::cout << "  Rx Packets   = " << tcpRxPkts << "\n";
        std::cout << "  Packet Loss  = " << tcpLoss << "\n";
        std::cout << "  Avg Delay    = " << tcpAvgDelay << " s\n";

        std::cout << "UDP:\n";
        std::cout << "  Tx Packets   = " << udpTxPkts << "\n";
        std::cout << "  Rx Packets   = " << udpRxPkts << "\n";
        std::cout << "  Packet Loss  = " << udpLoss << "\n";
        std::cout << "  Avg Delay    = " << udpAvgDelay << " s\n";

        std::cout << "Fairness Index (all flows) = " << fairness << "\n";

        Simulator::Destroy();
    }

    return 0;
}
