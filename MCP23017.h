#pragma once

#include "GPIO.h"


class MCP23017 : public GPIO {
public:
    MCP23017();
    MCP23017(bool pins[16]);

    bool Init(bool pins[16]);
    bool InitPin(uint8_t pin, bool direction);

    bool SetPin(uint8_t pin, bool state);
    bool SetAllPins(bool pins[16]);

    bool GetPin(uint8_t pin);
    bool[16] GetAllPins();

protected:



}
