
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include "common.h"

#pragma comment(lib, "Ws2_32.lib")

struct ClientConnection
{
    SOCKET socket;
    sockaddr_in addr;
    bool connected;
    char clientId[64];
};

class TCPServer
{
public:
    TCPServer();
    ~TCPServer();

    bool Start(const char* port = DEFAULT_PORT);
    void Stop();
    bool IsRunning() const;

    bool ReceiveMessage(NetworkMessage& message, ClientConnection** fromClient, int timeoutMs = 100);
    bool SendMessage(ClientConnection* client, const NetworkMessage& message);
    bool SendToAll(const NetworkMessage& message);
    size_t GetClientCount() const;
    void DisconnectClient(ClientConnection* client);

    // 新增：获取所有已断开连接的客户端（用于清理设备）
    std::vector<ClientConnection*> GetDisconnectedClients() const;
    // 新增：移除断开连接的客户端
    void RemoveDisconnectedClients();

private:
    bool InitializeWinsock();
    void CleanupWinsock();
    void AcceptConnections();

    SOCKET m_ListenSocket;
    bool m_Running;
    bool m_WinsockInitialized;
    std::vector<ClientConnection*> m_Clients;
};

#endif
