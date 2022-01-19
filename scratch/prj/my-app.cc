#include "my-app.h"
#include "my-mob.h"

#include "ns3/socket.h"
#include "ns3/uinteger.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("Drones");

NS_OBJECT_ENSURE_REGISTERED(DroneClient);
NS_OBJECT_ENSURE_REGISTERED(DroneServer);

// drone client

TypeId DroneClient::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::DroneClientApp")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<DroneClient>();
    return tid;
}

DroneClient::DroneClient(Address addr, uint16_t port) {
    NS_LOG_FUNCTION(this << Ipv4Address::ConvertFrom(addr) << port);
    m_remote_addr = addr;
    m_remote_port = port;
}

DroneClient::~DroneClient() {
    NS_LOG_FUNCTION(this);
}

void DroneClient::StartApplication() {
    NS_LOG_FUNCTION(this << "client");

    if (m_socket == 0) {
        auto tid = TypeId::LookupByName("ns3::TcpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        m_socket->Bind();
        if (m_socket->Connect(InetSocketAddress(
                Ipv4Address::ConvertFrom(m_remote_addr), m_remote_port)) < 0) {
            NS_FATAL_ERROR("DroneClient Connect failed: "
                           << Ipv4Address::ConvertFrom(m_remote_addr) << ":"
                           << m_remote_port);
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&DroneClient::HandleRead, this));

    auto pos = GetNode()->GetObject<MobilityModel>()->GetPosition();
    std::ostringstream ss;
    ss << "connect " << GetNode()->GetId() << " " << pos << std::endl;

    Send(ss.str());
}

int DroneClient::Send(std::string str) {
    if (m_socket != 0) {
        NS_LOG_FUNCTION(this << "client" << str);
        return m_socket->Send((const uint8_t *)&str[0], str.size(), 0);
    }
    return 0;
}

void DroneClient::HandleRead(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket << "client");

    auto mob    = GetNode()->GetObject<MobilityModel>();
    auto my_mob = DynamicCast<MyMob>(mob);

    Time               time;
    Vector             pos;
    std::string        cmd;
    std::ostringstream os;

    Ptr<Packet> packet;

    while ((packet = socket->Recv())) {
        packet->CopyData(&os, packet->GetSize());
        std::istringstream is(os.str());

        is >> cmd;

        if (cmd == "move") {
            is >> pos;
            is >> time;
            my_mob->MoveToWithin(pos, time);
            SetColor(0x0000ff);
        } else if (cmd == "delay") {
            is >> time;
            my_mob->Delay(time);
            SetColor(0xff0000);
        } else if (cmd == "accept") {
            Send("ready");
            SetColor(0x00ff00);
        }
    }
}

void DroneClient::StopApplication() {
    NS_LOG_FUNCTION(this << "client");

    if (m_socket != 0) {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_socket = 0;
    }
}

void DroneClient::SetNetani(AnimationInterface *netani) {
    this->netani = netani;
}

void DroneClient::SetColor(uint32_t hex) {
    if (netani != nullptr) {
        auto r = (hex & 0xff0000) >> 16;
        auto g = (hex & 0x00ff00) >> 8;
        auto b = (hex & 0x0000ff) >> 0;
        netani->UpdateNodeColor(GetNode(), r, g, b);
    }
}

// drone server

TypeId DroneServer::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::DroneServerApp")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<DroneServer>();
    return tid;
}

DroneServer::DroneServer(
    uint16_t port, uint16_t rows, uint16_t cols, double delx, double dely) {
    NS_LOG_FUNCTION(this << port);
    m_port = port;
    m_rows = rows;
    m_cols = cols;
    m_delx = delx;
    m_dely = dely;
}

DroneServer::~DroneServer() {
    NS_LOG_FUNCTION(this);
}

void DroneServer::StartApplication() {
    NS_LOG_FUNCTION(this << "server");

    if (m_socket == 0) {
        auto tid = TypeId::LookupByName("ns3::TcpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local =
            InetSocketAddress(Ipv4Address::GetAny(), m_port);
        auto res = m_socket->Bind(local);
        NS_LOG_INFO("DroneServer: " << local.GetIpv4() << ":" << m_port
                                    << " bind:" << res);
        if (res < 0) {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket->Listen();
    }

    m_socket->SetRecvCallback(MakeCallback(&DroneServer::HandleRead, this));
    m_socket->SetAcceptCallback(
        MakeCallback(&DroneServer::HandleAcceptRequest, this),
        MakeCallback(&DroneServer::HandleAccept, this));
    m_socket->SetCloseCallbacks(MakeCallback(&DroneServer::HandleClose, this),
        MakeCallback(&DroneServer::HandleClose, this));
}

int DroneServer::SendTo(Ptr<Socket> socket, std::string str) {
    if (m_socket != 0) {
        NS_LOG_FUNCTION(this << "server" << str);
        return socket->Send((const uint8_t *)&str[0], str.size(), 0);
    }
    return 0;
}

void DroneServer::HandleRead(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket << "server");

    std::string        cmd;
    std::ostringstream os;

    Ptr<Packet> packet;
    while ((packet = socket->Recv())) {
        packet->CopyData(&os, packet->GetSize());
        std::istringstream is(os.str());

        is >> cmd;

        if (cmd == "connect") {
            int    id;
            Vector pos;
            is >> id;
            is >> pos;
            m_clients[id] = {socket, pos};
            SendTo(socket, "accept\n");
        } else if (cmd == "ready") {
            if (m_clients.size() == m_rows * m_cols) {
                StartShow();
            }
        }
    }
}

void DroneServer::StartShow() {
    for (auto i = 0; i < m_rows; ++i) {
        for (auto j = 0; j < m_cols; ++j) {
            auto clnt = m_clients.find(i * m_rows + j);
            if (clnt == m_clients.end()) continue;
            Vector pos, cur = clnt->second.second;
            if (j % 2) {
                pos = Vector(cur.x - m_delx, cur.y - m_dely, cur.z);
            } else {
                pos = Vector(cur.x + m_delx, cur.y - m_dely, cur.z);
            }
            std::ostringstream os;
            os << "move " << pos << " " << Seconds(2) << std::endl;
            SendTo(clnt->second.first, os.str());
        }
    }
}

void DroneServer::HandleAccept(Ptr<Socket> socket, const Address &from) {
    NS_LOG_FUNCTION(
        this << socket << InetSocketAddress::ConvertFrom(from).GetIpv4());
    socket->SetRecvCallback(MakeCallback(&DroneServer::HandleRead, this));
}

bool DroneServer::HandleAcceptRequest(Ptr<Socket> socket, const Address &from) {
    NS_LOG_FUNCTION(this << socket << from);
    return true;
}

void DroneServer::HandleClose(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket << "server");

    for (auto it = m_clients.begin(); it != m_clients.end(); it++) {
        if (it->second.first == socket) {
            m_clients.erase(it->first);
            break;
        }
    }
}

void DroneServer::StopApplication() {
    NS_LOG_FUNCTION(this << "server");
}

} // namespace ns3
