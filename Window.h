#pragma once

#include "State.h"


class Window {
public:
    Window(const uint8_t pinOpen, const uint8_t pinClose, const uint32_t path, const uint32_t timeToOpen /* us */, const uint32_t timeToClose /* us */);
    ~Window() = default;

    
    // Public Member Methods
    bool Toogle();
    bool Open();
    bool Close();

    bool ToogleState();
    bool OpenState();
    bool CloseState();


private:
    // Private Member Variables
    uint8_t m_pinOpen;
    uint8_t m_pinClose;
    State m_state;
    uint32_t m_timeToOpen; // us
    uint32_t m_timeToClose; // us


    // Private Member Methods
    bool OpenHardware();
    bool CloseHardware();

    void ThreadSafeSleep(const uint32_t delay /* us */);


};
