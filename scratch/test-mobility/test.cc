#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/netanim-module.h"

#include "my-mob.h"

using namespace ns3;
using PtrC = Ptr<MyMob>;

NS_LOG_COMPONENT_DEFINE("test-mobility");

void show_info(PtrC &m0, PtrC &m1, PtrC &m2) {
    NS_LOG_INFO("vel0: " << m0->GetVelocity()
                         << "\tpos0: " << m0->GetPosition());
    NS_LOG_INFO("vel1: " << m1->GetVelocity()
                         << "\tpos1: " << m1->GetPosition());
    NS_LOG_INFO("vel2: " << m2->GetVelocity()
                         << "\tpos2: " << m2->GetPosition());
    std::cout << std::endl;
    Simulator::Schedule(Seconds(1), &show_info, m0, m1, m2);
}

void change_vel(PtrC &m0, PtrC &m1) {
    auto mod = Now() % Seconds(4);
    if (mod == Seconds(0)) {
        m0->SetVelocity(Vector(10, 0, 0));
        m1->SetVelocity(Vector(-10, 0, 0));
    } else if (mod == Seconds(1)) {
        m0->SetVelocity(Vector(0, 10, 0));
        m1->SetVelocity(Vector(0, -10, 0));
    } else if (mod == Seconds(2)) {
        m0->SetVelocity(Vector(-10, 0, 0));
        m1->SetVelocity(Vector(10, 0, 0));
    } else if (mod == Seconds(3)) {
        m0->SetVelocity(Vector(0, -10, 0));
        m1->SetVelocity(Vector(0, 10, 0));
    }

    Simulator::Schedule(Seconds(1), &change_vel, m0, m1);
}

int main(int ac, char **av) {
    LogComponentEnable("test-mobility", LOG_LEVEL_INFO);
    CommandLine cmd;
    cmd.Parse(ac, av);

    NodeContainer nodes;
    nodes.Create(3);

    MobilityHelper mob;
    mob.SetMobilityModel("ns3::MyMob");
    mob.InstallAll();

    auto m0 = DynamicCast<MyMob>(nodes.Get(0)->GetObject<MobilityModel>());
    auto m1 = DynamicCast<MyMob>(nodes.Get(1)->GetObject<MobilityModel>());
    auto m2 = DynamicCast<MyMob>(nodes.Get(2)->GetObject<MobilityModel>());

    m0->SetPosition(Vector(0, 10, 0));
    m1->SetPosition(Vector(30, 20, 0));

    m2->SetPosition(Vector(0, 0, 0));
    m2->MoveToWithin(Vector(0, 30, 0), Seconds(1));
    m2->MoveToWithin(Vector(30, 30, 0), Seconds(1));
    m2->Delay(Seconds(2));
    m2->MoveToWithin(Vector(30, 0, 0), Seconds(1));
    m2->MoveToWith(Vector(0, 0, 0), 5);

    // Simulator::Schedule(Seconds(0), &show_info, m0, m1, m2);
    Simulator::Schedule(Seconds(0), &change_vel, m0, m1);

    AnimationInterface ani("test-mobility.xml");
    Simulator::Stop(Seconds(60));
    Simulator::Run();
    Simulator::Destroy();
}
