
#include <iostream>
#include <thread>
#include <chrono>
#include "xbox_controller.h"
#include "tcp_client.h"

int main()
{
    std::cout << "=== TCP Client ===" << std::endl;

    // 初始化 Xbox 手柄
    XboxController controller;
    if (!controller.Initialize())
    {
        std::cerr << "Failed to initialize Xbox controller" << std::endl;
        return 1;
    }

    if (!controller.IsConnected())
    {
        std::cerr << "Xbox controller not connected" << std::endl;
        return 1;
    }

    // 初始化 TCP 客户端
    TCPClient client;
    if (!client.Connect())
    {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    std::cout << "Press ESC to exit" << std::endl;

    XboxControllerState lastState = { 0 };
    while (true)
    {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            std::cout << "ESC pressed, exiting..." << std::endl;
            break;
        }

        XboxControllerState state;
        if (controller.GetControllerState(state))
        {
            if (memcmp(&state, &lastState, sizeof(XboxControllerState)) != 0)
            {
                if (client.SendXboxState(state))
                {
                    std::cout << "Sent state" << std::endl;
                }
                memcpy(&lastState, &state, sizeof(XboxControllerState));
            }
        }
        else
        {
            std::cerr << "Controller disconnected" << std::endl;
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
                std::cout << "Server disconnect" << std::endl;
                client.Disconnect();
                return 1;
            default:
                std::cerr << "Unknown message type: " << msg.header.type << std::endl;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    client.Disconnect();
    controller.Shutdown();

    return 0;
}
