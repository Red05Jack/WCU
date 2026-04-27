#pragma once

#include "GPIO.h"


// I/O-Expander 16 GPIOs
class MCP23017 : public GPIO {
public:
    MCP23017();
    MCP23017(Pin pins[16]);


    bool Init(bool pins[16]);
    bool InitPin(uint8_t pin, bool direction);

    bool SetPin(Pin pin);
    bool SetAllPins(Pin pins[16]);

    bool GetPin(uint8_t pin);
    bool[16] GetAllPins();

protected:



};
