#include "ns3/simulator.h"
#include "my-mob.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(MyMob);

TypeId MyMob::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::MyMob")
                            .SetParent<MobilityModel>()
                            .SetGroupName("Mobility")
                            .AddConstructor<MyMob>();
    return tid;
}

MyMob::MyMob() {
}

MyMob::~MyMob() {
}

inline Vector MyMob::DoGetVelocity(void) const {
    return m_baseVelocity;
}

inline Vector MyMob::DoGetPosition(void) const {
    double t = (Simulator::Now() - m_baseTime).GetSeconds();
    return Vector(m_basePosition.x + m_baseVelocity.x * t,
        m_basePosition.y + m_baseVelocity.y * t,
        m_basePosition.z + m_baseVelocity.z * t);
}

void MyMob::DoSetPosition(const Vector &position) {
    m_baseTime     = Simulator::Now();
    m_basePosition = position;
    NotifyCourseChange();
}

void MyMob::SetVelocity(const Vector &velocity) {
    m_basePosition = DoGetPosition();
    m_baseTime     = Simulator::Now();
    m_baseVelocity = velocity;
    NotifyCourseChange();
}

void MyMob::Next(void) {
    m_basePosition = DoGetPosition();
    m_cur_process  = nullptr;
    m_baseVelocity = Vector(0, 0, 0);
    m_queue.pop_front();
    StartQueue();
}

void MyMob::StartQueue(void) {
    if (m_queue.empty() || m_cur_process != nullptr) return;

    m_cur_process  = &m_queue.front();
    m_basePosition = DoGetPosition();
    m_baseTime     = Simulator::Now();

    auto vel       = m_cur_process->pos - m_basePosition;
    auto time      = m_cur_process->time.GetSeconds();
    m_baseVelocity = Vector(vel.x / time, vel.y / time, vel.z / time);

    Simulator::Schedule(m_cur_process->time, &MyMob::Next, this);

    NotifyCourseChange();
}

void MyMob::MoveToWithin(const Vector &pos, const Time &in) {
    m_queue.push_back(PosAndTime{pos, in});
    StartQueue();
    NotifyCourseChange();
}

void MyMob::MoveToWith(const Vector &pos, double vel) {
    if (vel <= 0) return;

    auto last_pos = m_queue.empty() ? DoGetPosition() : m_queue.back().pos;
    auto len      = (pos - last_pos).GetLength();
    auto time     = len / vel;

    MoveToWithin(pos, Seconds(time));

    NotifyCourseChange();
}

void MyMob::Delay(const Time &time) {
    auto last_pos = m_queue.empty() ? DoGetPosition() : m_queue.back().pos;
    MoveToWithin(last_pos, time);
    NotifyCourseChange();
}

} // namespace ns3
