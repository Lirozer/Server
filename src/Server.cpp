#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>

#include "Constants.h"

#include "Server.h"

void Server::Start()
{
    Status createTcpSocketStatus = CreateTcpSocket();

    if (Status::Success != createTcpSocketStatus)
    {
        return;
    }

    Status createUdpSocketStatus = CreateUdpSocket();

    if (Status::Success != createUdpSocketStatus)
    {
        return;
    }

    Status bindTcpSocketStatus = BindSocket(tcpSocket);

    if (Status::Success != bindTcpSocketStatus)
    {
        return;
    }

    Status bindUdpSocketStatus = BindSocket(udpSocket);

    if (Status::Success != bindUdpSocketStatus)
    {
        return;
    }

    Status listenSocketStatus = ListenTcpSocket();

    if (Status::Success != listenSocketStatus)
    {
        return;
    }

    Status CreateEpollStatus = CreateEpoll();

    if (Status::Success != CreateEpollStatus)
    {
        return;
    }

    Status AddEpollTcpSocketStatus = AddEpollSocket(tcpSocket);

    if (Status::Success != AddEpollTcpSocketStatus)
    {
        return;
    }

    Status AddEpollUdpSocketStatus = AddEpollSocket(udpSocket);

    if (Status::Success != AddEpollUdpSocketStatus)
    {
        return;
    }

    Run();
}

Status Server::CreateTcpSocket()
{
    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 == tcpSocket)
    {
        std::cerr << "Произошла ошибка при создании TCP-сокета" << std::endl;
        return Status::SocketCreationFailed;
    }

    SetupSocket(tcpSocket);

    std::cout << "TCP-Сокет создан: " + std::to_string(tcpSocket) << std::endl;

    return Status::Success;
}

Status Server::CreateUdpSocket()
{
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (-1 == udpSocket)
    {
        std::cerr << "Произошла ошибка при создании UDP-сокета" << std::endl;
        return Status::SocketCreationFailed;
    }

    SetupSocket(udpSocket);

    std::cout << "UDP-Сокет создан: " + std::to_string(udpSocket) << std::endl;

    return Status::Success;
}

void Server::SetupSocket(int socket)
{
    int reuseOption = 1;
    socklen_t reuseOptionSize = sizeof(reuseOption);
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuseOption, reuseOptionSize);

    int socketOptions = fcntl(socket, F_GETFL);
    fcntl(socket, F_SETFL, socketOptions | O_NONBLOCK);
}

Status Server::BindSocket(int socket)
{
    sockaddr_in socketAddress{};
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(PORT);

    sockaddr *genericSocketAddress = reinterpret_cast<sockaddr*>(&socketAddress);
    socklen_t socketAddressLenght = sizeof(socketAddress);

    int bindResult = bind(socket, genericSocketAddress, socketAddressLenght);

    if (-1 == bindResult)
    {
        close(socket);

        std::cerr << "Произошла ошибка при привязке сокета " + std::to_string(socket) + " к порту " + std::to_string(PORT) << std::endl;
        return Status::SocketBindFailed;
    }

    std::cout << "Сокет " + std::to_string(socket) + " привязан к порту " + std::to_string(PORT) << std::endl;

    return Status::Success;
}

Status Server::ListenTcpSocket()
{
    int listenResult = listen(tcpSocket, NUMBER_CONNECTION);

    if (-1 == listenResult)
    {
        close(tcpSocket);

        std::cerr << "Произошла ошибка при прослушивании на порту " + std::to_string(PORT) << std::endl;
        return Status::SocketListenFailed;
    }

    std::cout << "Сервер слушает подключения на порту " + std::to_string(PORT) << std::endl;

    return Status::Success;
}

Status Server::CreateEpoll()
{
    epoll = epoll_create1(0);

    if (-1 == epoll)
    {
        std::cerr << "Ошибка создания epoll" << std::endl;
        return Status::EpollCreationFailed;
    }

    return Status::Success;
}

Status Server::AddEpollSocket(int socket)
{
    epoll_event epollEvent;
    epollEvent.events = EPOLLIN;
    epollEvent.data.fd = socket;

    int epollAddResult = epoll_ctl(epoll, EPOLL_CTL_ADD, socket, &epollEvent);

    if (-1 == epollAddResult)
    {
        std::cerr << "Ошибка регистрации сокета " + std::to_string(socket) << std::endl;
        return Status::EpollAddFailed;
    }

    return Status::Success;
}

void Server::Run()
{
    epoll_event events[MAX_EVENTS]{};

    while (true)
    {
        int eventCount = epoll_wait(epoll, events, MAX_EVENTS, WAIT_TIME);

        for (int eventNumber = 0; eventNumber < eventCount; eventNumber++)
        {
            int socket = events[eventNumber].data.fd;

            if (socket == tcpSocket)
            {
                AcceptConnection();
            }
            else if (socket == udpSocket)
            {
                HandleUdpPacket();
            }
            else
            {
                HandleTcpClient(socket);
            }
        }
    }
}

Status Server::AcceptConnection()
{
    int clientSocket = accept(tcpSocket, nullptr, nullptr);

    if (-1 == clientSocket)
    {
        std::cerr << "Ошибка создания клиентского сокета" << std::endl;
        return Status::SocketCreationFailed;
    }

    int socketOption = fcntl(clientSocket, F_GETFL);
    fcntl(clientSocket, F_SETFL, socketOption | O_NONBLOCK);

    Status addEpollSocketStatus = AddEpollSocket(clientSocket);

    if (Status::EpollAddFailed == addEpollSocketStatus)
    {
        return Status::EpollAddFailed;
    }

    totalConnections++;
    currentConnections++;

    std::cout << "Клиент подключился! Сокет: " + std::to_string(clientSocket) << std::endl;

    return Status::Success;
}

void Server::HandleTcpClient(int socket)
{
    char messageBuffer[MESSAGE_BUFFER_SIZE]{};

    size_t messageLength = recv(socket, messageBuffer, MESSAGE_BUFFER_SIZE, 0);

    if (0 == messageLength)
    {
        DisconnectClient(socket);
        return;
    }

    if (messageLength > MESSAGE_BUFFER_SIZE)
    {
        return;
    }

    std::string requestMessage(messageBuffer);
    NormalizeMessage(requestMessage);

    if (Command::DISCONNECT == requestMessage)
    {
        DisconnectClient(socket);
        return;
    }

    std::string responseMessage = "";
    GenerateResponseMessage(requestMessage, responseMessage);

    const char *responseMessageData = responseMessage.c_str();
    size_t responseMessageLength = responseMessage.length();

    send(socket, responseMessageData, responseMessageLength, 0);
}

void Server::HandleUdpPacket()
{
    char messageBuffer[MESSAGE_BUFFER_SIZE]{};

    sockaddr clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);

    size_t messageLength = recvfrom(udpSocket, messageBuffer, MESSAGE_BUFFER_SIZE, 0, &clientAddress, &clientAddressLength);

    if (0 == messageLength)
    {
        return;
    }

    if (messageLength > MESSAGE_BUFFER_SIZE)
    {
        return;
    }

    std::string requestMessage(messageBuffer);
    NormalizeMessage(requestMessage);

    if (Command::DISCONNECT == requestMessage)
    {
        return;
    }

    std::string responseMessage = "";
    GenerateResponseMessage(requestMessage, responseMessage);

    const char *responseMessageData = responseMessage.c_str();
    size_t responseMessageLength = responseMessage.length();

    sendto(udpSocket, responseMessageData, responseMessageLength, 0, &clientAddress, clientAddressLength);
}

void Server::NormalizeMessage(std::string &message)
{
    char lastCharacter = message.back();

    while (('\r' == lastCharacter) || ('\n' == lastCharacter))
    {
        message.pop_back();
        lastCharacter = message.back();
    }
}

void Server::GenerateResponseMessage(std::string &requestMessage, std::string &responseMessage)
{
    if (Command::TIME == requestMessage)
    {
        GetTimeMessage(responseMessage);
    }
    else if (Command::STATS == requestMessage)
    {
        responseMessage = "Текущих подключений: " + std::to_string(currentConnections) + "\t Всего подключений: " + std::to_string(totalConnections) + "\r\n";
    }
    else
    {
        responseMessage = requestMessage + "\r\n";
    }
}

void Server::GetTimeMessage(std::string &message)
{
    time_t currentTime = time(nullptr);

    tm *localTime = std::localtime(&currentTime);
    char timeBuffer[MESSAGE_BUFFER_SIZE];

    std::strftime(timeBuffer, MESSAGE_BUFFER_SIZE, TIME_FORMAT, localTime);

    message = "Текущее время: " + std::string(timeBuffer) + "\r\n";
}

void Server::DisconnectClient(int socket)
{
    epoll_ctl(epoll, EPOLL_CTL_DEL, socket, nullptr);
    currentConnections--;

    std::cout << "Клиент отключился! Сокет: " << socket << std::endl;

    close(socket);
}