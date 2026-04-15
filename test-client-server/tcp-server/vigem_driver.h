#pragma once

#include "common.h"
#include <ViGEm/Client.h>
#include <vector>

#pragma comment(lib, "setupapi.lib")

// 前向声明
struct ClientConnection;

struct VirtualController
{
    PVIGEM_TARGET target;
    DWORD index;
    ClientConnection* client;
    bool connected;
};

class ViGEmDriver
{
public:
    ViGEmDriver();
    ~ViGEmDriver();

    bool Initialize();
    void Shutdown();

    bool IsReady() const { return m_Initialized; }

    VirtualController* CreateXbox360Controller(ClientConnection* client);
    bool RemoveController(VirtualController* controller);
    bool UpdateXbox360State(VirtualController* controller, const XboxControllerState& state);

    bool GetVibration(VirtualController* controller, XboxVibrationState& state);
    bool SetVibration(VirtualController* controller, const XboxVibrationState& state);

    size_t GetControllerCount() const { return m_Controllers.size(); }
    std::vector<VirtualController*> GetControllersByClient(ClientConnection* client = nullptr);

private:
    void CleanupControllers();
    static DWORD GetNextFreeSlot();

    static VOID CALLBACK X360VibrationCallback(
        PVIGEM_CLIENT Client,
        PVIGEM_TARGET Target,
        UCHAR LargeMotor,
        UCHAR SmallMotor,
        UCHAR LedNumber,
        LPVOID UserData
    );

    PVIGEM_CLIENT       m_Client;
    bool                m_Initialized;
    std::vector<VirtualController*> m_Controllers;
    static DWORD        m_NextSlot;
};
