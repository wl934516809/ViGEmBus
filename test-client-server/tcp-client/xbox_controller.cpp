
#include "xbox_controller.h"
#include <Xinput.h>
#include <iostream>

XboxController::XboxController(int userIndex)
    : m_UserIndex(userIndex)
    , m_Connected(false)
    , m_ControllerName("Xbox Controller")
{
}

XboxController::~XboxController()
{
    Shutdown();
}

bool XboxController::Initialize()
{
    XINPUT_STATE state;
    memset(&state, 0, sizeof(XINPUT_STATE));

    DWORD result = XInputGetState(m_UserIndex, &state);

    if (result == ERROR_SUCCESS)
    {
        m_Connected = true;
        LOG_INFO("XBOX Controller " << m_UserIndex << " connected");
        return true;
    }
    else
    {
        m_Connected = false;
        LOG_INFO("XBOX Controller " << m_UserIndex << " not found");
        return false;
    }
}

void XboxController::Shutdown()
{
    m_Connected = false;
}

bool XboxController::IsConnected() const
{
    return m_Connected;
}

bool XboxController::GetControllerState(XboxControllerState& state)
{
    if (!m_Connected)
        return false;

    XINPUT_STATE xstate;
    memset(&xstate, 0, sizeof(XINPUT_STATE));

    DWORD result = XInputGetState(m_UserIndex, &xstate);

    if (result == ERROR_SUCCESS)
    {
        state.wButtons = xstate.Gamepad.wButtons;
        state.bLeftTrigger = xstate.Gamepad.bLeftTrigger;
        state.bRightTrigger = xstate.Gamepad.bRightTrigger;
        state.sThumbLX = xstate.Gamepad.sThumbLX;
        state.sThumbLY = xstate.Gamepad.sThumbLY;
        state.sThumbRX = xstate.Gamepad.sThumbRX;
        state.sThumbRY = xstate.Gamepad.sThumbRY;

        return true;
    }
    else
    {
        m_Connected = false;
        return false;
    }
}

int XboxController::GetUserIndex() const
{
    return m_UserIndex;
}

bool XboxController::SetVibration(uint16_t leftMotor, uint16_t rightMotor)
{
    if (!m_Connected)
        return false;

    XINPUT_VIBRATION vib;
    vib.wLeftMotorSpeed = leftMotor;
    vib.wRightMotorSpeed = rightMotor;

    DWORD result = XInputSetState(m_UserIndex, &vib);

    return (result == ERROR_SUCCESS);
}

const std::string& XboxController::GetControllerName() const
{
    return m_ControllerName;
}
