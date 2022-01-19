#pragma once

#include "ns3/vector.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"
#include "ns3/netanim-module.h"

using namespace ns3;

namespace ns3 {
class Socket;

class DroneClient : public Application {
  public:
    static TypeId GetTypeId(void);
    DroneClient(Address addr = Address(), uint16_t port = 8080);
    ~DroneClient();
    int  Send(std::string str);
    void SetNetani(AnimationInterface *netani);
    void SetColor(uint32_t hex);

  private:
    virtual void StartApplication();
    virtual void StopApplication();
    void         HandleRead(Ptr<Socket> socket);

    Address             m_remote_addr;
    uint16_t            m_remote_port;
    Ptr<Socket>         m_socket;
    AnimationInterface *netani;
};

class DroneServer : public Application {
  public:
    static TypeId GetTypeId(void);
    DroneServer(uint16_t port = 8080, uint16_t rows = 7, uint16_t cols = 8,
        double delx = 5, double dely = 5);
    ~DroneServer();
    int SendTo(Ptr<Socket> socket, std::string str);

  private:
    virtual void StartApplication();
    virtual void StopApplication();
    void         HandleRead(Ptr<Socket> socket);
    void         HandleAccept(Ptr<Socket> socket, const Address &from);
    bool         HandleAcceptRequest(Ptr<Socket> socket, const Address &from);
    void         HandleClose(Ptr<Socket> socket);
    void         StartShow();

    uint16_t                                      m_port;
    Ptr<Socket>                                   m_socket;
    uint16_t                                      m_rows;
    uint16_t                                      m_cols;
    double                                        m_delx;
    double                                        m_dely;
    std::map<int, std::pair<Ptr<Socket>, Vector>> m_clients;
};

} // namespace ns3
