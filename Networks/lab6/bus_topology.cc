// ========================= bus_topology.cc =========================
// Bus topology implemented with CSMA (single LAN subnet)

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  uint32_t N = 10;
  double simTime = 100.0;
  std::string csvPrefix = "results_bus";

  CommandLine cmd;
  cmd.AddValue ("N", "Number of nodes", N);
  cmd.Parse (argc, argv);

  if (N < 2)
    {
      std::cout << "Need at least 2 nodes\n";
      return 1;
    }

  NodeContainer nodes;
  nodes.Create (N);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer devices = csma.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ifs = address.Assign (devices);

  // Applications: odd -> next even
  uint16_t port = 50000;
  for (uint32_t i = 1; i+1 <= N; i += 2)
    {
      // destination is node i (0-based index i)
      Address sinkLocalAddress (InetSocketAddress (ifs.GetAddress (i), port));
      PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
      ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get (i));
      sinkApp.Start (Seconds (0.0));
      sinkApp.Stop (Seconds (simTime + 1.0));

      OnOffHelper onoff ("ns3::UdpSocketFactory", Address (sinkLocalAddress));
      onoff.SetConstantRate (DataRate ("1Mbps"));
      onoff.SetAttribute ("PacketSize", UintegerValue (1024));
      onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

      ApplicationContainer srcApp = onoff.Install (nodes.Get (i - 1));
      srcApp.Start (Seconds (0.0));
      srcApp.Stop (Seconds (simTime));
    }

  // Flow monitor
  FlowMonitorHelper flowmonHelper;
  Ptr<FlowMonitor> flowmon = flowmonHelper.InstallAll ();

  Simulator::Stop (Seconds (simTime + 1.0));
  Simulator::Run ();

  // Analyze FlowMonitor
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats ();

  uint64_t totalRxBytes = 0;
  uint64_t totalTxPackets = 0;
  uint64_t totalRxPackets = 0;

  for (auto &kv : stats)
    {
      FlowId id = kv.first;
      FlowMonitor::FlowStats st = kv.second;
      // std::string flowDesc = classifier->FindFlow (id).c_str ();
      totalRxBytes += st.rxBytes;
      totalTxPackets += st.txPackets;
      totalRxPackets += st.rxPackets;
    }

  double throughput_bps = (double) totalRxBytes * 8.0 / simTime; // bps
  double throughput_mbps = throughput_bps / 1e6;
  uint64_t packetLoss = 0;
  if (totalTxPackets >= totalRxPackets) packetLoss = totalTxPackets - totalRxPackets;

  // Output CSV
  std::ostringstream fname;
  fname << csvPrefix << "_" << N << ".csv";
  std::ofstream ofs (fname.str ());
  ofs << "Nodes,Throughput_Mbps,PacketLoss\n";
  ofs << N << "," << throughput_mbps << "," << packetLoss << "\n";
  ofs.close ();

  // Also print summary
  std::cout << "BUS TOPOLOGY - N=" << N << " Throughput(Mbps): " << throughput_mbps << " PacketLoss: " << packetLoss << "\n";

  Simulator::Destroy ();
  return 0;
}