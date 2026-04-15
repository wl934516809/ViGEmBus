
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include "xbox_controller.h"
#include "tcp_client.h"

void ShowUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -i, --ip <address>    Server IP address (default: " << DEFAULT_SERVER_ADDR << ")" << std::endl;
    std::cout << "  -p, --port <port>      Server port (default: " << DEFAULT_PORT << ")" << std::endl;
    std::cout << "  -h, --help             Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << programName << " --ip 192.168.1.100 --port 12345" << std::endl;
}

int main(int argc, char* argv[])
{
    std::string serverAddr = DEFAULT_SERVER_ADDR;
    std::string serverPort = DEFAULT_PORT;

    // 解析命令行参数
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            ShowUsage(argv[0]);
            return 0;
        }
        else if ((arg == "-i" || arg == "--ip") && i + 1 < argc)
        {
            serverAddr = argv[++i];
        }
        else if ((arg == "-p" || arg == "--port") && i + 1 < argc)
        {
            serverPort = argv[++i];
        }
        else
        {
            LOG_ERROR("Unknown option: " << arg);
            ShowUsage(argv[0]);
            return 1;
        }
    }

    LOG_INFO("=== TCP Client ===");
    LOG_INFO("Server: " << serverAddr << ":" << serverPort);

    // 初始化 Xbox 手柄
    XboxController controller;
    if (!controller.Initialize())
    {
        LOG_ERROR("Failed to initialize Xbox controller");
        return 1;
    }

    if (!controller.IsConnected())
    {
        LOG_ERROR("Xbox controller not connected");
        return 1;
    }

    // 初始化 TCP 客户端
    TCPClient client;
    if (!client.Connect(serverAddr.c_str(), serverPort.c_str()))
    {
        LOG_ERROR("Failed to connect to server");
        return 1;
    }

    LOG_INFO("Press ESC to exit");

    XboxControllerState lastState = { 0 };
    while (true)
    {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            LOG_INFO("ESC pressed, exiting...");
            break;
        }

        XboxControllerState state;
        if (controller.GetControllerState(state))
        {
            if (memcmp(&state, &lastState, sizeof(XboxControllerState)) != 0)
            {
                if (client.SendXboxState(state))
                {
                    LOG_IO("Sent state");
                }
                memcpy(&lastState, &state, sizeof(XboxControllerState));
            }
        }
        else
        {
            LOG_ERROR("Controller disconnected");
            break;
        }

        NetworkMessage msg;
        if (client.ReceiveMessage(msg, 10))
        {
            switch (msg.header.type)
            {
            case MSG_XBOX_VIBRATION:
                controller.SetVibration(msg.data.vibration.wLeftMotorSpeed,
                    msg.data.vibration.wRightMotorSpeed);
                break;
            case MSG_HEARTBEAT:
                break;
            case MSG_DISCONNECT:
                LOG_INFO("Server disconnect");
                client.Disconnect();
                return 1;
            default:
                LOG_ERROR("Unknown message type: " << msg.header.type);
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    client.Disconnect();
    controller.Shutdown();

    return 0;
}
