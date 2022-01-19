#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/netanim-module.h"

#include "my-app.h"
#include "my-mob.h"

using namespace ns3;

int main(int ac, char **av) {
    LogComponentEnable("Drones", LOG_LEVEL_ALL);

    CommandLine cmd;
    cmd.Parse(ac, av);

    NodeContainer nodes;
    nodes.Create(2);

    MobilityHelper mob;
    mob.SetMobilityModel("ns3::MyMob");
    mob.SetPositionAllocator(
        "ns3::GridPositionAllocator", "MinX", DoubleValue(0), "MinY",
        DoubleValue(10), "DeltaX", DoubleValue(20), "DeltaY", DoubleValue(20),
        "GridWidth", UintegerValue(4), "LayoutType", StringValue("RowFirst"));
    mob.InstallAll();

    PointToPointHelper p2pHelper;
    p2pHelper.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2pHelper.SetChannelAttribute("Delay", StringValue("2ms"));

    auto devs = p2pHelper.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.0.1.0", "255.255.255.0");
    auto itf = address.Assign(devs);

    auto server = CreateObject<DroneServer>(8080, 1, 1, 5, 5);
    auto client = CreateObject<DroneClient>(itf.GetAddress(0));

    server->SetStartTime(Seconds(0));
    client->SetStartTime(Seconds(1));

    server->SetStopTime(Seconds(10));
    client->SetStopTime(Seconds(8));

    nodes.Get(0)->AddApplication(server);
    nodes.Get(1)->AddApplication(client);

    AnimationInterface ani("test-app.xml");
    ani.SkipPacketTracing();
    ani.SetMobilityPollInterval(Seconds(0.001));
    Simulator::Stop(Seconds(10));
    Simulator::Run();
    Simulator::Destroy();
}
