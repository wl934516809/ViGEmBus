
#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "common.h"

#pragma comment(lib, "Ws2_32.lib")

class TCPClient
{
public:
    TCPClient();
    ~TCPClient();

    bool Connect(const char* serverAddr = DEFAULT_SERVER_ADDR, const char* port = DEFAULT_PORT);
    void Disconnect();
    bool IsConnected() const;

    bool SendXboxState(const XboxControllerState& state);
    bool SendVibration(const XboxVibrationState& state);
    bool SendHeartbeat();
    bool ReceiveMessage(NetworkMessage& message, int timeoutMs = 100);

private:
    bool SendMessage(const NetworkMessage& message);
    bool InitializeWinsock();
    void CleanupWinsock();

    SOCKET          m_Socket;
    bool            m_Connected;
    bool            m_WinsockInitialized;
};

#endif
