#include <ns3/core-module.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/ipv4-address-helper.h>
#include <ns3/csma-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/udp-echo-helper.h>
#include <ns3/ipv4-global-routing-helper.h>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("sher");

int main(int ac, char **av) {
    uint32_t nCsma = 4;

    CommandLine cmd;
    cmd.AddValue("nCsma", "num of csma", nCsma);

    cmd.Parse(ac, av);

    nCsma = nCsma < 0 ? 0 : nCsma;

    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    NodeContainer p2p;
    p2p.Create(2);

    NodeContainer csma;
    csma.Add(p2p.Get(1));
    csma.Create(nCsma);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    auto p2pNet = pointToPoint.Install(p2p);

    CsmaHelper csmaH;
    csmaH.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csmaH.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    auto csmaNet = csmaH.Install(csma);

    InternetStackHelper stack;
    stack.Install(p2p.Get(0));
    stack.Install(csma);

    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");
    auto p2pI = address.Assign(p2pNet);

    address.SetBase("10.0.1.0", "255.255.255.0");
    auto csmaI = address.Assign(csmaNet);

    UdpEchoServerHelper echoServer(8080);
    auto server = echoServer.Install(csma.Get(nCsma));
    server.Start(Seconds(1));
    server.Stop(Seconds(10));

    UdpEchoClientHelper echoClient(csmaI.GetAddress(nCsma), 8080);
    echoClient.SetAttribute("MaxPackets", UintegerValue(8));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    auto client = echoClient.Install(p2p.Get(0));
    client.Start(Seconds(2));
    client.Stop(Seconds(10));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    AsciiTraceHelper ascii;
    Ipv4GlobalRoutingHelper::PrintRoutingTableAt(
        Seconds(8), csma.Get(1), ascii.CreateFileStream("sher.routes"));

    pointToPoint.EnablePcapAll("sher");
    csmaH.EnablePcap("sher", csmaNet.Get(1), true);

    Simulator::Run();
    Simulator::Destroy();
}
