#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include "vigem_driver.h"
#include "tcp_server.h"

#include <iostream>
#include <algorithm>

DWORD ViGEmDriver::m_NextSlot = 0;

ViGEmDriver::ViGEmDriver()
    : m_Client(nullptr)
    , m_Initialized(false)
{
}

ViGEmDriver::~ViGEmDriver()
{
    Shutdown();
}

bool ViGEmDriver::Initialize()
{
    if (m_Initialized)
        return true;

    m_Client = vigem_alloc();
    if (m_Client == nullptr)
    {
        LOG_ERROR("Failed to allocate ViGEm client");
        return false;
    }

    VIGEM_ERROR error = vigem_connect(m_Client);
    if (!VIGEM_SUCCESS(error))
    {
        LOG_ERROR("Failed to connect to ViGEm driver, error: 0x" << std::hex << error << std::dec);
        LOG_ERROR("Please ensure ViGEmBus driver is installed!");
        vigem_free(m_Client);
        m_Client = nullptr;
        return false;
    }

    m_Initialized = true;
    LOG_INFO("Connected to ViGEmBus driver");
    return true;
}

void ViGEmDriver::Shutdown()
{
    if (!m_Initialized)
        return;

    CleanupControllers();

    if (m_Client != nullptr)
    {
        vigem_disconnect(m_Client);
        vigem_free(m_Client);
        m_Client = nullptr;
    }

    m_Initialized = false;
    LOG_INFO("Disconnected from ViGEmBus driver");
}

DWORD ViGEmDriver::GetNextFreeSlot()
{
    return m_NextSlot++;
}

VirtualController* ViGEmDriver::CreateXbox360Controller(ClientConnection* client)
{
    if (!m_Initialized || m_Client == nullptr)
    {
        LOG_ERROR("ViGEm driver not initialized");
        return nullptr;
    }

    VirtualController* controller = new VirtualController();
    controller->target = vigem_target_x360_alloc();
    controller->client = client;
    controller->index = GetNextFreeSlot();
    controller->connected = false;

    if (controller->target == nullptr)
    {
        LOG_ERROR("Failed to allocate Xbox 360 controller");
        delete controller;
        return nullptr;
    }

    VIGEM_ERROR error = vigem_target_add(m_Client, controller->target);
    if (!VIGEM_SUCCESS(error))
    {
        LOG_ERROR("Failed to add Xbox 360 controller, error: 0x" << std::hex << error << std::dec);
        vigem_target_free(controller->target);
        delete controller;
        return nullptr;
    }

    error = vigem_target_x360_register_notification(
        m_Client,
        controller->target,
        &ViGEmDriver::X360VibrationCallback,
        controller
    );

    controller->connected = true;
    m_Controllers.push_back(controller);

    LOG_IO("Created Xbox 360 controller #" << controller->index);

    return controller;
}

bool ViGEmDriver::RemoveController(VirtualController* controller)
{
    if (!m_Initialized || controller == nullptr)
        return false;

    auto it = std::find(m_Controllers.begin(), m_Controllers.end(), controller);
    if (it == m_Controllers.end())
        return false;

    if (controller->connected && controller->target != nullptr)
    {
        vigem_target_remove(m_Client, controller->target);
        vigem_target_free(controller->target);
    }

    LOG_IO("Removed Xbox 360 controller #" << controller->index);

    delete controller;
    m_Controllers.erase(it);

    return true;
}

void ViGEmDriver::CleanupControllers()
{
    LOG_DEBUG("Cleaning up " << m_Controllers.size() << " virtual controllers...");

    for (auto controller : m_Controllers)
    {
        if (controller->connected && controller->target != nullptr)
        {
            vigem_target_remove(m_Client, controller->target);
            vigem_target_free(controller->target);
        }
        delete controller;
    }

    m_Controllers.clear();
}

bool ViGEmDriver::UpdateXbox360State(VirtualController* controller, const XboxControllerState& state)
{
    if (!m_Initialized || controller == nullptr || !controller->connected)
        return false;

    if (controller->target == nullptr)
        return false;

    const XUSB_REPORT* report = reinterpret_cast<const XUSB_REPORT*>(&state);
    VIGEM_ERROR error = vigem_target_x360_update(m_Client, controller->target, *report);

    return VIGEM_SUCCESS(error);
}

bool ViGEmDriver::GetVibration(VirtualController* controller, XboxVibrationState& state)
{
    if (!m_Initialized || controller == nullptr)
        return false;

    state.wLeftMotorSpeed = 0;
    state.wRightMotorSpeed = 0;

    return true;
}

bool ViGEmDriver::SetVibration(VirtualController* controller, const XboxVibrationState& state)
{
    return false;
}

VOID CALLBACK ViGEmDriver::X360VibrationCallback(
    PVIGEM_CLIENT Client,
    PVIGEM_TARGET Target,
    UCHAR LargeMotor,
    UCHAR SmallMotor,
    UCHAR LedNumber,
    LPVOID UserData
)
{
    VirtualController* controller = static_cast<VirtualController*>(UserData);
    if (controller == nullptr || controller->client == nullptr)
        return;

    XboxVibrationState state;
    state.wLeftMotorSpeed = static_cast<uint16_t>(LargeMotor) * 257;
    state.wRightMotorSpeed = static_cast<uint16_t>(SmallMotor) * 257;

    LOG_IO("Vibration: large=" << static_cast<int>(LargeMotor)
              << ", small=" << static_cast<int>(SmallMotor)
              << ", LED=" << static_cast<int>(LedNumber));
}

std::vector<VirtualController*> ViGEmDriver::GetControllersByClient(ClientConnection* client)
{
    std::vector<VirtualController*> result;

    for (auto controller : m_Controllers)
    {
        if (client == nullptr || controller->client == client)
        {
            result.push_back(controller);
        }
    }

    return result;
}
