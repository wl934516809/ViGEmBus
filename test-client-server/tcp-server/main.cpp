#include "tcp_server.h"
#include "vigem_driver.h"
#include <iostream>
#include <thread>
#include <chrono>

void PrintMessageInfo(const NetworkMessage& message, ClientConnection* fromClient)
{
    std::cout << "Received message type: ";

    switch (message.header.type)
    {
        case MSG_HEARTBEAT:
            std::cout << "HEARTBEAT";
            break;
        case MSG_XBOX_STATE:
            std::cout << "XBOX_STATE";
            break;
        case MSG_XBOX_VIBRATION:
            std::cout << "XBOX_VIBRATION";
            break;
        case MSG_DISCONNECT:
            std::cout << "DISCONNECT";
            break;
        default:
            std::cout << "UNKNOWN";
            break;
    }

    std::cout << ", length: " << message.header.length;
    std::cout << ", from: " << fromClient->clientId;
    std::cout << " [" << inet_ntoa(fromClient->addr.sin_addr) << ":"
              << ntohs(fromClient->addr.sin_port) << "]" << std::endl;
}

void PrintXboxState(const XboxControllerState& state)
{
    std::cout << "  ";
    if (state.wButtons != 0) std::cout << "BUTTONS=" << std::hex << state.wButtons << " ";
    if (state.bLeftTrigger != 0) std::cout << "LTRIG=" << (int)state.bLeftTrigger << " ";
    if (state.bRightTrigger != 0) std::cout << "RTRIG=" << (int)state.bRightTrigger << " ";
    if (state.sThumbLX != 0) std::cout << "LX=" << state.sThumbLX << " ";
    if (state.sThumbLY != 0) std::cout << "LY=" << state.sThumbLY << " ";
    if (state.sThumbRX != 0) std::cout << "RX=" << state.sThumbRX << " ";
    if (state.sThumbRY != 0) std::cout << "RY=" << state.sThumbRY << " ";
    std::cout << std::dec << std::endl;
}

int main()
{
    std::cout << "=== ViGEm Server ===" << std::endl;
    std::cout << "Initializing..." << std::endl;

    TCPServer server;

    if (!server.Start())
    {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    ViGEmDriver vigem;

    if (!vigem.Initialize())
    {
        std::cerr << "Failed to connect to ViGEm driver" << std::endl;
        server.Stop();
        return 1;
    }

    std::cout << "\n=== Server Ready ===" << std::endl;
    std::cout << "Press ESC to stop server" << std::endl;
    std::cout << "--------------------------\n" << std::endl;

    NetworkMessage message;
    ClientConnection* fromClient;

    while (true)
    {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            std::cout << "\n\nESC pressed, stopping server..." << std::endl;
            break;
        }

        if (server.ReceiveMessage(message, &fromClient))
        {
            PrintMessageInfo(message, fromClient);

            switch (message.header.type)
            {
                case MSG_HEARTBEAT:
                    std::cout << "  Processing heartbeat" << std::endl;
                    break;

                case MSG_XBOX_STATE:
                {
                    std::cout << "  Processing Xbox state: ";
                    PrintXboxState(message.data.controller);

                    auto controllers = vigem.GetControllersByClient(fromClient);
                    VirtualController* controller = nullptr;

                    if (controllers.empty())
                    {
                        controller = vigem.CreateXbox360Controller(fromClient);
                    }
                    else
                    {
                        controller = controllers[0];
                    }

                    if (controller != nullptr)
                    {
                        if (!vigem.UpdateXbox360State(controller, message.data.controller))
                        {
                            std::cerr << "Failed to update virtual controller state" << std::endl;
                        }
                    }

                    break;
                }

                case MSG_XBOX_VIBRATION:
                    {
                        std::cout << "  Processing vibration: "
                                  << "Left=" << message.data.vibration.wLeftMotorSpeed
                                  << ", Right=" << message.data.vibration.wRightMotorSpeed << std::endl;

                        auto controllers = vigem.GetControllersByClient(fromClient);
                        if (!controllers.empty())
                        {
                            vigem.SetVibration(controllers[0], message.data.vibration);
                        }
                    }
                    break;

                case MSG_DISCONNECT:
                    {
                        std::cout << "  Client is disconnecting" << std::endl;
                        server.DisconnectClient(fromClient);

                        auto clientControllers = vigem.GetControllersByClient(fromClient);
                        for (auto controller : clientControllers)
                        {
                            vigem.RemoveController(controller);
                        }
                    }
                    break;

                default:
                    std::cout << "  Unknown message type: " << message.header.type << std::endl;
                    break;
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // 定期检查并清理断开连接的客户端的设备
        for (size_t i = 0; i < server.GetClientCount(); )
        {
            // 我们无法直接访问内部，所以我们需要另一种方法
            // 让我们检查所有客户端的控制器，如果客户端已断开，就删除控制器
            // 先获取所有现有的控制器
            auto allControllers = vigem.GetControllersByClient(nullptr);

            // 对于每个控制器，检查其关联的客户端是否还连接
            for (auto controller : allControllers)
            {
                if (controller && controller->client)
                {
                    // 如果客户端已断开，删除控制器
                    if (!controller->client->connected)
                    {
                        std::cout << "Cleaning up device for disconnected client..." << std::endl;
                        vigem.RemoveController(controller);
                    }
                }
            }

            break; // 只处理一次
        }
    }

    server.Stop();
    vigem.Shutdown();

    std::cout << "\n=== Server Stopped ===" << std::endl;
    std::cout << "All resources released" << std::endl;
    std::cout << "Press any key to continue..." << std::endl;
    std::cin.get();

    return 0;
}
