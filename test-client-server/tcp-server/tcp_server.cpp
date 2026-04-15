
#include "tcp_server.h"
#include <iostream>
#include <thread>
#include <algorithm>

TCPServer::TCPServer()
    : m_ListenSocket(INVALID_SOCKET)
    , m_Running(false)
    , m_WinsockInitialized(false)
{
}

TCPServer::~TCPServer()
{
    Stop();
}

bool TCPServer::InitializeWinsock()
{
    if (m_WinsockInitialized)
        return true;

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        LOG_ERROR("WSAStartup failed: " << iResult);
        return false;
    }

    m_WinsockInitialized = true;
    return true;
}

void TCPServer::CleanupWinsock()
{
    if (m_WinsockInitialized)
    {
        WSACleanup();
        m_WinsockInitialized = false;
    }
}

bool TCPServer::Start(const char* port)
{
    if (!InitializeWinsock())
        return false;

    m_ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_ListenSocket == INVALID_SOCKET)
    {
        LOG_ERROR("Socket creation failed: " << WSAGetLastError());
        CleanupWinsock();
        return false;
    }

    int optval = 1;
    setsockopt(m_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((u_short)atoi(port));

    if (bind(m_ListenSocket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        LOG_ERROR("Bind failed: " << WSAGetLastError());
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
        CleanupWinsock();
        return false;
    }

    if (listen(m_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        LOG_ERROR("Listen failed: " << WSAGetLastError());
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
        CleanupWinsock();
        return false;
    }

    m_Running = true;

    LOG_INFO("Server listening on port " << port);
    LOG_INFO("Press ESC to stop");

    return true;
}

void TCPServer::Stop()
{
    if (!m_Running)
        return;

    m_Running = false;

    if (m_ListenSocket != INVALID_SOCKET)
    {
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
    }

    for (auto client : m_Clients)
    {
        if (client->connected)
        {
            NetworkMessage msg;
            FillMessageHeader(&msg.header, MSG_DISCONNECT, 0);
            send(client->socket, (const char*)&msg, sizeof(MessageHeader), 0);
            closesocket(client->socket);
        }
        delete client;
    }
    m_Clients.clear();

    CleanupWinsock();

    LOG_INFO("Server stopped");
}

bool TCPServer::IsRunning() const
{
    return m_Running;
}

void TCPServer::AcceptConnections()
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_ListenSocket, &readSet);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    int selectResult = select((int)m_ListenSocket + 1, &readSet, nullptr, nullptr, &timeout);

    if (selectResult > 0 && FD_ISSET(m_ListenSocket, &readSet))
    {
        sockaddr_in clientAddr;
        int addrSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(m_ListenSocket, (struct sockaddr*)&clientAddr, &addrSize);

        if (clientSocket != INVALID_SOCKET)
        {
            ClientConnection* client = new ClientConnection();
            client->socket = clientSocket;
            client->addr = clientAddr;
            client->connected = true;

            sprintf_s(client->clientId, 64, "Client_%08X", (unsigned int)client->socket);

            m_Clients.push_back(client);

            LOG_IO(client->clientId << " connected from "
                << inet_ntoa(clientAddr.sin_addr) << ":"
                << ntohs(clientAddr.sin_port));

            LOG_IO("Total clients: " << m_Clients.size());
        }
    }
}

void TCPServer::DisconnectClient(ClientConnection* client)
{
    if (client->connected)
    {
        client->connected = false;

        NetworkMessage msg;
        FillMessageHeader(&msg.header, MSG_DISCONNECT, 0);
        send(client->socket, (const char*)&msg, sizeof(MessageHeader), 0);
        closesocket(client->socket);

        LOG_IO(client->clientId << " disconnected");
    }

    // 从客户端列表中删除
    auto it = std::find(m_Clients.begin(), m_Clients.end(), client);
    if (it != m_Clients.end())
    {
        delete *it;
        m_Clients.erase(it);
        LOG_IO("Total clients: " << m_Clients.size());
    }
}

bool TCPServer::ReceiveMessage(NetworkMessage& message, ClientConnection** fromClient, int timeoutMs)
{
    AcceptConnections();

    if (m_Clients.empty())
        return false;

    fd_set readSet;
    FD_ZERO(&readSet);

    SOCKET maxSocket = m_ListenSocket;
    for (auto client : m_Clients)
    {
        if (client->connected)
        {
            FD_SET(client->socket, &readSet);
            if (client->socket > maxSocket)
                maxSocket = client->socket;
        }
    }

    timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    int selectResult = select((int)maxSocket + 1, &readSet, nullptr, nullptr, &timeout);

    if (selectResult == SOCKET_ERROR)
    {
        LOG_ERROR("Select failed: " << WSAGetLastError());
        return false;
    }

    if (selectResult == 0)
        return false;

    for (size_t i = 0; i < m_Clients.size(); ++i)
    {
        ClientConnection* client = m_Clients[i];

        if (client->connected && FD_ISSET(client->socket, &readSet))
        {
            int iResult = recv(client->socket, (char*)&message.header, sizeof(MessageHeader), 0);

            if (iResult == SOCKET_ERROR)
            {
                LOG_ERROR("Receive header failed: " << WSAGetLastError());
                DisconnectClient(client);
                continue;
            }

            if (iResult == 0)
            {
                LOG_IO(client->clientId << " closed connection");
                DisconnectClient(client);
                continue;
            }

            if (!IsValidMessage(&message.header))
            {
                LOG_ERROR("Invalid message from " << client->clientId);
                continue;
            }

            if (message.header.length > 0 && message.header.length <= MAX_BUFFER_SIZE)
            {
                iResult = recv(client->socket, (char*)&message.data,
                    message.header.length, 0);

                if (iResult == SOCKET_ERROR)
                {
                    LOG_ERROR("Receive data failed: " << WSAGetLastError());
                    DisconnectClient(client);
                    continue;
                }
            }

            *fromClient = client;
            return true;
        }
    }

    return false;
}

bool TCPServer::SendMessage(ClientConnection* client, const NetworkMessage& message)
{
    if (!client->connected)
        return false;

    int iResult = send(client->socket, (const char*)&message,
        sizeof(MessageHeader) + message.header.length, 0);

    if (iResult == SOCKET_ERROR)
    {
        LOG_ERROR("Send failed to " << client->clientId << ": " << WSAGetLastError());
        DisconnectClient(client);
        return false;
    }

    return true;
}

bool TCPServer::SendToAll(const NetworkMessage& message)
{
    bool allSent = true;

    for (auto client : m_Clients)
    {
        if (!SendMessage(client, message))
            allSent = false;
    }

    return allSent;
}

size_t TCPServer::GetClientCount() const
{
    return m_Clients.size();
}
