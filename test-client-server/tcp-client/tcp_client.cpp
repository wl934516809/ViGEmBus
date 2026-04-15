
#include "tcp_client.h"
#include <iostream>

TCPClient::TCPClient()
    : m_Socket(INVALID_SOCKET)
    , m_Connected(false)
    , m_WinsockInitialized(false)
{
}

TCPClient::~TCPClient()
{
    Disconnect();
}

bool TCPClient::InitializeWinsock()
{
    if (m_WinsockInitialized)
        return true;

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return false;
    }

    m_WinsockInitialized = true;
    return true;
}

void TCPClient::CleanupWinsock()
{
    if (m_WinsockInitialized)
    {
        WSACleanup();
        m_WinsockInitialized = false;
    }
}

bool TCPClient::Connect(const char* serverAddr, const char* port)
{
    if (!InitializeWinsock())
        return false;

    if (m_Connected)
        Disconnect();

    m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_Socket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        CleanupWinsock();
        return false;
    }

    sockaddr_in serverAddrInfo;
    serverAddrInfo.sin_family = AF_INET;
    serverAddrInfo.sin_port = htons((u_short)atoi(port));
    if (inet_pton(AF_INET, serverAddr, &serverAddrInfo.sin_addr) <= 0)
    {
        std::cerr << "Invalid server address: " << serverAddr << std::endl;
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
        return false;
    }

    int iResult = connect(m_Socket, (struct sockaddr*)&serverAddrInfo, sizeof(serverAddrInfo));
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
        return false;
    }

    m_Connected = true;
    std::cout << "Connected to " << serverAddr << ":" << port << std::endl;

    SendHeartbeat();

    return true;
}

void TCPClient::Disconnect()
{
    if (m_Connected)
    {
        NetworkMessage msg;
        FillMessageHeader(&msg.header, MSG_DISCONNECT, 0);
        SendMessage(msg);

        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
        m_Connected = false;
        std::cout << "Disconnected from server" << std::endl;
    }

    CleanupWinsock();
}

bool TCPClient::IsConnected() const
{
    return m_Connected;
}

bool TCPClient::SendMessage(const NetworkMessage& message)
{
    if (!m_Connected)
        return false;

    int iResult = send(m_Socket, (const char*)&message,
        sizeof(MessageHeader) + message.header.length, 0);

    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        m_Connected = false;
        return false;
    }

    return true;
}

bool TCPClient::SendXboxState(const XboxControllerState& state)
{
    NetworkMessage msg;
    FillMessageHeader(&msg.header, MSG_XBOX_STATE, sizeof(XboxControllerState));
    msg.data.controller = state;

    return SendMessage(msg);
}

bool TCPClient::SendVibration(const XboxVibrationState& state)
{
    NetworkMessage msg;
    FillMessageHeader(&msg.header, MSG_XBOX_VIBRATION, sizeof(XboxVibrationState));
    msg.data.vibration = state;

    return SendMessage(msg);
}

bool TCPClient::SendHeartbeat()
{
    NetworkMessage msg;
    FillMessageHeader(&msg.header, MSG_HEARTBEAT, 0);
    return SendMessage(msg);
}

bool TCPClient::ReceiveMessage(NetworkMessage& message, int timeoutMs)
{
    if (!m_Connected)
        return false;

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_Socket, &readSet);

    timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    int selectResult = select(static_cast<int>(m_Socket) + 1, &readSet, NULL, NULL, &timeout);

    if (selectResult == SOCKET_ERROR)
    {
        std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
        m_Connected = false;
        return false;
    }

    if (selectResult == 0)
        return false;

    int iResult = recv(m_Socket, (char*)&message.header, sizeof(MessageHeader), 0);
    if (iResult == SOCKET_ERROR)
    {
        std::cerr << "Receive header failed: " << WSAGetLastError() << std::endl;
        m_Connected = false;
        return false;
    }

    if (iResult == 0)
    {
        m_Connected = false;
        return false;
    }

    if (!IsValidMessage(&message.header))
    {
        std::cerr << "Invalid message header" << std::endl;
        return false;
    }

    if (message.header.length > 0 && message.header.length <= MAX_BUFFER_SIZE)
    {
        iResult = recv(m_Socket, (char*)&message.data, message.header.length, 0);
        if (iResult == SOCKET_ERROR)
        {
            std::cerr << "Receive data failed: " << WSAGetLastError() << std::endl;
            m_Connected = false;
            return false;
        }
    }

    return true;
}
