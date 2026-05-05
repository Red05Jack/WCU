#pragma once

#include <cstdint>
#include <array>

#include "I2C.h"

enum class Mcp23017PinMode : uint8_t {
  Output = 0,
  Input = 1
};

struct Mcp23017Pin {
  uint8_t pin = 0;
  Mcp23017PinMode mode = Mcp23017PinMode::Input;
  bool state = false;
};

class MCP23017 {
public:
  MCP23017();
  MCP23017(I2C* i2c, uint8_t address = 0x20);

  bool Init(I2C* i2c, uint8_t address = 0x20);
  bool InitPin(uint8_t pin, Mcp23017PinMode mode);

  bool SetPin(uint8_t pin, bool state);
  bool SetPin(const Mcp23017Pin& pin);

  bool SetAllPins(const std::array<bool, 16>& states);

  bool GetPin(uint8_t pin, bool& state);
  bool GetAllPins(std::array<bool, 16>& states);

  bool SetPullUp(uint8_t pin, bool enabled);

private:
  static constexpr uint8_t RegisterIodirA = 0x00;
  static constexpr uint8_t RegisterIodirB = 0x01;
  static constexpr uint8_t RegisterGpioA = 0x12;
  static constexpr uint8_t RegisterGpioB = 0x13;
  static constexpr uint8_t RegisterOlatA = 0x14;
  static constexpr uint8_t RegisterOlatB = 0x15;

  static constexpr uint8_t RegisterGppuA = 0x0C;
static constexpr uint8_t RegisterGppuB = 0x0D;

  bool WriteRegister(uint8_t reg, uint8_t value);
  bool ReadRegister(uint8_t reg, uint8_t& value);

  bool IsValidPin(uint8_t pin) const;

private:
  I2C* m_i2c = nullptr;
  uint8_t m_address = 0x20;

  uint8_t m_directionA = 0xFF;
  uint8_t m_directionB = 0xFF;

  uint8_t m_outputA = 0x00;
  uint8_t m_outputB = 0x00;

  bool m_isInitialized = false;

  uint8_t m_pullUpA = 0x00;
uint8_t m_pullUpB = 0x00;
};