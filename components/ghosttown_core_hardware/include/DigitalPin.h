#pragma once

#include "driver/gpio.h"

class DigitalPin {
public:
  virtual ~DigitalPin() = default;

  virtual bool Set(bool state) = 0;
  virtual bool Get(bool& state) = 0;

  virtual bool SetMode(bool isOutput) = 0;
  virtual bool SetPullUp(bool enabled) = 0;

  virtual bool AttachInterrupt(gpio_int_type_t interruptType, void (*callback)(void*), void* argument = nullptr) = 0;
  virtual bool DetachInterrupt() = 0;
};
