#pragma once

#include "ns3/mobility-model.h"
#include "ns3/nstime.h"

#include <deque>

namespace ns3 {
struct PosAndTime {
    Vector pos;
    Time   time;
};

class MyMob : public MobilityModel {
  public:
    static TypeId GetTypeId(void);
    MyMob();
    virtual ~MyMob();
    void SetVelocity(const Vector &velocity);
    void MoveToWithin(const Vector &pos, const Time &in);
    void MoveToWith(const Vector &pos, const double vel);
    void Delay(const Time &time);

  private:
    virtual Vector DoGetPosition(void) const;
    virtual Vector DoGetVelocity(void) const;
    virtual void   DoSetPosition(const Vector &position);
    virtual void   StartQueue(void);
    virtual void   Next(void);

    Time   m_baseTime;
    Vector m_basePosition;
    Vector m_baseVelocity;

    std::deque<PosAndTime> m_queue;
    PosAndTime            *m_cur_process = nullptr;
};

} // namespace ns3
