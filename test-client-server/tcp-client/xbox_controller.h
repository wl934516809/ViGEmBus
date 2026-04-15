
#ifndef XBOX_CONTROLLER_H
#define XBOX_CONTROLLER_H

#include "common.h"
#include <string>

class XboxController
{
public:
    XboxController(int userIndex = 0);
    ~XboxController();

    bool Initialize();
    void Shutdown();

    bool IsConnected() const;
    bool GetControllerState(XboxControllerState& state);
    int GetUserIndex() const;

    bool SetVibration(uint16_t leftMotor, uint16_t rightMotor);
    const std::string& GetControllerName() const;

private:
    int             m_UserIndex;
    bool            m_Connected;
    std::string     m_ControllerName;
};

#endif
