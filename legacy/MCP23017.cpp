#include "MCP23017.h"


bool MCP23017::SetAllPins(bool pins[16]) {
    for(int i = 0; i < 16; i++) {
        gpio_set_level(i, pins[i]);
    }

    return true;
}