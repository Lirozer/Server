#include <string>

#include "Status.h"

class Server
{
public:
    Server() = default;
    ~Server() = default;

    void Start();

private:
    int tcpSocket = -1;
    int udpSocket = -1;

    int epoll = -1;

    int totalConnections = 0;
    int currentConnections = 0;

    Status CreateTcpSocket();
    Status CreateUdpSocket();
    void SetupSocket(int socket);

    Status BindSocket(int socket);
    Status ListenTcpSocket();

    Status CreateEpoll();
    Status AddEpollSocket(int socket);

    void Run();

    Status AcceptConnection();

    void HandleTcpClient(int socket);
    void HandleUdpPacket();

    void NormalizeMessage(std::string &message);
    void GenerateResponseMessage(std::string &requestMessage, std::string &responseMessage);

    void GetTimeMessage(std::string &message);
    void DisconnectClient(int socket);
};