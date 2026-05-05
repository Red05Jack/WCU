#include "Window.h"



uint32_t Window::GetTimeOpen() {
  uint8_t high = state.GetState(config.timeOpenHighPos);
  uint8_t low = state.GetState(config.timeOpenLowPos);

  return ((((uint16_t)high << 8) | low) * 10);
}

uint32_t Window::GetTimeClose() {
  uint8_t high = state.GetState(config.timeCloseHighPos);
  uint8_t low = state.GetState(config.timeCloseLowPos);

  return ((((uint16_t)high << 8) | low) * 10);
}

bool Window::SetTimeOpen(uint32_t time) {
    if (time > 655350) {
        time = 655350;
    }

    uint16_t raw = time / 10;

    uint8_t high = (raw >> 8) & 0xFF;
    uint8_t low = raw & 0xFF;

    bool isHighSet = state.SetState(high, config.timeOpenHighPos);
    bool isLowSet = state.SetState(low, config.timeOpenLowPos);

    return isHighSet && isLowSet;
}


bool Window::SetTimeClose(uint32_t time) {
    if (time > 655350) {
        time = 655350;
    }

    uint16_t raw = time / 10;

    uint8_t high = (raw >> 8) & 0xFF;
    uint8_t low = raw & 0xFF;

    bool isHighSet = state.SetState(high, config.timeCloseHighPos);
    bool isLowSet = state.SetState(low, config.timeCloseLowPos);

    return isHighSet && isLowSet;
}
