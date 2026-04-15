#include "tcp_server.h"
#include "vigem_driver.h"
#include <iostream>
#include <thread>
#include <chrono>

void PrintMessageInfo(const NetworkMessage& message, ClientConnection* fromClient)
{
    LOG_IO("Received message type: ");

    switch (message.header.type)
    {
        case MSG_HEARTBEAT:
            LOG_IO("HEARTBEAT");
            break;
        case MSG_XBOX_STATE:
            LOG_IO("XBOX_STATE");
            break;
        case MSG_XBOX_VIBRATION:
            LOG_IO("XBOX_VIBRATION");
            break;
        case MSG_DISCONNECT:
            LOG_IO("DISCONNECT");
            break;
        default:
            LOG_IO("UNKNOWN");
            break;
    }

    LOG_IO(", length: " << message.header.length);
    LOG_IO(", from: " << fromClient->clientId);
    LOG_IO(" [" << inet_ntoa(fromClient->addr.sin_addr) << ":"
              << ntohs(fromClient->addr.sin_port) << "]");
}

void PrintXboxState(const XboxControllerState& state)
{
    LOG_IO("  ");
    if (state.wButtons != 0) LOG_IO("BUTTONS=" << std::hex << state.wButtons << " ");
    if (state.bLeftTrigger != 0) LOG_IO("LTRIG=" << (int)state.bLeftTrigger << " ");
    if (state.bRightTrigger != 0) LOG_IO("RTRIG=" << (int)state.bRightTrigger << " ");
    if (state.sThumbLX != 0) LOG_IO("LX=" << state.sThumbLX << " ");
    if (state.sThumbLY != 0) LOG_IO("LY=" << state.sThumbLY << " ");
    if (state.sThumbRX != 0) LOG_IO("RX=" << state.sThumbRX << " ");
    if (state.sThumbRY != 0) LOG_IO("RY=" << state.sThumbRY << " ");
    LOG_IO(std::dec << "");
}

int main()
{
    LOG_INFO("=== ViGEm Server ===");
    LOG_INFO("Initializing...");

    TCPServer server;

    if (!server.Start())
    {
        LOG_ERROR("Failed to start server");
        return 1;
    }

    ViGEmDriver vigem;

    if (!vigem.Initialize())
    {
        LOG_ERROR("Failed to connect to ViGEm driver");
        server.Stop();
        return 1;
    }

    LOG_INFO("\n=== Server Ready ===");
    LOG_INFO("Press ESC to stop server");
    LOG_INFO("--------------------------\n");

    NetworkMessage message;
    ClientConnection* fromClient;

    while (true)
    {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            LOG_INFO("\n\nESC pressed, stopping server...");
            break;
        }

        if (server.ReceiveMessage(message, &fromClient))
        {
            PrintMessageInfo(message, fromClient);

            switch (message.header.type)
            {
                case MSG_HEARTBEAT:
                    LOG_IO("  Processing heartbeat");
                    break;

                case MSG_XBOX_STATE:
                {
                    LOG_IO("  Processing Xbox state: ");
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
                            LOG_ERROR("Failed to update virtual controller state");
                        }
                    }

                    break;
                }

                case MSG_XBOX_VIBRATION:
                    {
                        LOG_IO("  Processing vibration: "
                                  << "Left=" << message.data.vibration.wLeftMotorSpeed
                                  << ", Right=" << message.data.vibration.wRightMotorSpeed);

                        auto controllers = vigem.GetControllersByClient(fromClient);
                        if (!controllers.empty())
                        {
                            vigem.SetVibration(controllers[0], message.data.vibration);
                        }
                    }
                    break;

                case MSG_DISCONNECT:
                    {
                        LOG_IO("  Client is disconnecting");
                        server.DisconnectClient(fromClient);

                        auto clientControllers = vigem.GetControllersByClient(fromClient);
                        for (auto controller : clientControllers)
                        {
                            vigem.RemoveController(controller);
                        }
                    }
                    break;

                default:
                    LOG_IO("  Unknown message type: " << message.header.type);
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
                        LOG_DEBUG("Cleaning up device for disconnected client...");
                        vigem.RemoveController(controller);
                    }
                }
            }

            break; // 只处理一次
        }
    }

    server.Stop();
    vigem.Shutdown();

    LOG_INFO("\n=== Server Stopped ===");
    LOG_INFO("All resources released");
    LOG_INFO("Press any key to continue...");
    std::cin.get();

    return 0;
}
