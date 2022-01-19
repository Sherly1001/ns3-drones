#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"

#include "ns3/mobility-helper.h"
#include "my-mob.h"
#include "my-app.h"

using namespace ns3;

void drop(Ptr<Packet> p) {
    std::cout << "PhyRxDrop at " << Now().GetSeconds() << " " << p << std::endl;
}

int main(int ac, char **av) {
    auto delx = 5;
    auto dely = 5;
    auto rows = 7;
    auto cols = 8;
    auto simt = 30;

    // LogComponentEnable("Drones", LOG_LEVEL_ALL);

    CommandLine cmd;
    cmd.AddValue("delx", "with spacing", delx);
    cmd.AddValue("dely", "height spacing", dely);
    cmd.AddValue("rows", "number row of drones", rows);
    cmd.AddValue("cols", "number column of drones", cols);
    cmd.AddValue("simt", "simulator time", simt);
    cmd.Parse(ac, av);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(rows * cols);

    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    auto wifiApNode = p2pNodes.Get(0);
    auto serverNode = p2pNodes.Get(1);

    PointToPointHelper p2pHelper;
    p2pHelper.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2pHelper.SetChannelAttribute("Delay", StringValue("1ms"));

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(YansWifiChannelHelper::Default().Create());

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    auto ssid = Ssid("ns-3-ssid");

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing",
        BooleanValue(false));

    auto p2pDevs = p2pHelper.Install(p2pNodes);

    auto wifiStaDevs = wifi.Install(wifiPhy, wifiMac, wifiStaNodes);
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    auto wifiApDev = wifi.Install(wifiPhy, wifiMac, wifiApNode);

    MobilityHelper mobHelper;
    mobHelper.SetPositionAllocator("ns3::GridPositionAllocator", "MinX",
        DoubleValue(0), "MinY", DoubleValue(0), "DeltaX", DoubleValue(delx),
        "DeltaY", DoubleValue(dely), "GridWidth", UintegerValue(cols),
        "LayoutType", StringValue("RowFirst"));

    mobHelper.SetMobilityModel("ns3::MyMob");

    mobHelper.Install(wifiStaNodes);
    mobHelper.Install(wifiApNode);
    mobHelper.Install(serverNode);

    auto wifiApMob = DynamicCast<MyMob>(wifiApNode->GetObject<MobilityModel>());
    auto serverMob = DynamicCast<MyMob>(serverNode->GetObject<MobilityModel>());

    wifiApMob->SetPosition(Vector((cols * 0.5 - 1) * delx, rows * dely, 0));
    serverMob->SetPosition(Vector((cols * 0.5) * delx, rows * dely, 0));

    InternetStackHelper stack;
    stack.Install(wifiStaNodes);
    stack.Install(wifiApNode);
    stack.Install(serverNode);

    Ipv4AddressHelper address;

    address.SetBase("10.0.1.0", "255.255.255.0");
    auto p2pItf = address.Assign(p2pDevs);

    address.SetBase("10.0.2.0", "255.255.255.0");
    auto wifiStaItf = address.Assign(wifiStaDevs);
    auto wifiApItf  = address.Assign(wifiApDev);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    AnimationInterface ani("prj.xml");
    ani.SkipPacketTracing();

    ApplicationContainer clntApps;
    for (auto i = 0; i < wifiStaNodes.GetN(); ++i) {
        auto addr = p2pItf.GetAddress(1);
        auto clnt = CreateObject<DroneClient>(addr, 8080);
        clnt->SetNetani(&ani);
        wifiStaNodes.Get(i)->AddApplication(clnt);
        clntApps.Add(clnt);
    }

    clntApps.Start(Seconds(1));
    clntApps.Stop(Seconds(simt));

    auto server = CreateObject<DroneServer>(8080, rows, cols, delx, dely);
    server->SetStartTime(Seconds(0));
    server->SetStopTime(Seconds(simt));
    p2pNodes.Get(1)->AddApplication(server);

    wifiPhy.EnablePcap("phy", NodeContainer(wifiApNode));
    wifiApDev.Get(0)->TraceConnectWithoutContext(
        "PhyTxDrop", MakeCallback(&drop));
    p2pHelper.EnablePcapAll("p2p");

    Simulator::Stop(Seconds(simt));
    Simulator::Run();
    Simulator::Destroy();
}
