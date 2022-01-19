#include <ns3/core-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/network-module.h>
#include <ns3/applications-module.h>
#include <ns3/mobility-module.h>
#include <ns3/csma-module.h>
#include <ns3/internet-module.h>
#include <ns3/yans-wifi-helper.h>
#include <ns3/ssid.h>
#include <ns3/netanim-module.h>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("wifi");

void sher(std::string ctx, Ptr<const MobilityModel> model) {
    auto pos = model->GetPosition();
    NS_LOG_UNCOND(ctx << ": x: " << pos.x << ", y: " << pos.y);
}

int main(int argc, char *argv[]) {
    uint32_t nCsma = 3;
    uint32_t nWifi = 3;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue("nWifi", "Number of wifi STA devices", nWifi);

    cmd.Parse(argc, argv);

    if (nWifi > 18) {
        std::cout << "nWifi should be 18 or less; otherwise grid layout "
                     "exceeds the bounding box"
                  << std::endl;
        return 1;
    }

    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    auto p2pDevices = pointToPoint.Install(p2pNodes);

    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1));
    csmaNodes.Create(nCsma);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    auto csmaDevices = csma.Install(csmaNodes);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);
    NodeContainer wifiApNode = p2pNodes.Get(0);

    auto channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    auto ssid = Ssid("ns-3-ssid");
    mac.SetType("ns3::StaWifiMac",
        "Ssid", SsidValue(ssid),
        "ActiveProbing", BooleanValue(false));

    auto staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    auto apDevices = wifi.Install(phy, mac, wifiApNode);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
        "MinX", DoubleValue(0),
        "MinY", DoubleValue(0),
        "DeltaX", DoubleValue(20),
        "DeltaY", DoubleValue(20),
        "GridWidth", UintegerValue(3),
        "LayoutType", StringValue("RowFirst"));

    // mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
    //     "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));

    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiStaNodes);
    mobility.Install(wifiApNode);
    mobility.Install(csmaNodes);

    InternetStackHelper stack;
    stack.Install(csmaNodes);
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;

    address.SetBase("10.0.1.0", "255.255.255.0");
    auto p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.0.2.0", "255.255.255.0");
    auto csmaInterfaces = address.Assign(csmaDevices);

    address.SetBase("10.0.3.0", "255.255.255.0");
    address.Assign(staDevices);
    address.Assign(apDevices);

    UdpEchoServerHelper echoServer(8080);
    auto serverApps = echoServer.Install(csmaNodes.Get(nCsma));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(csmaInterfaces.GetAddress(nCsma), 8080);
    echoClient.SetAttribute("MaxPackets", UintegerValue(8));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    auto clientApps = echoClient.Install(wifiStaNodes.Get(nWifi - 1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    pointToPoint.EnablePcapAll("wifi");
    phy.EnablePcap("wifi", apDevices.Get(0));
    csma.EnablePcap("wifi", csmaDevices.Get(0), true);

    AnimationInterface anim("wifi.xml");

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
