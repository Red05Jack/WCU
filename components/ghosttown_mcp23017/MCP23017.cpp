#include "MCP23017.h"

MCP23017::MCP23017() {
}

MCP23017::MCP23017(I2C* i2c, uint8_t address) {
  Init(i2c, address);
}

bool MCP23017::Init(I2C* i2c, uint8_t address) {
  if (i2c == nullptr) {
    return false;
  }

  m_i2c = i2c;
  m_address = address;

  m_directionA = 0xFF;
  m_directionB = 0xFF;
  m_outputA = 0x00;
  m_outputB = 0x00;

  if (!WriteRegister(RegisterIodirA, m_directionA)) {
    return false;
  }

  if (!WriteRegister(RegisterIodirB, m_directionB)) {
    return false;
  }

  if (!WriteRegister(RegisterOlatA, m_outputA)) {
    return false;
  }

  if (!WriteRegister(RegisterOlatB, m_outputB)) {
    return false;
  }

  m_isInitialized = true;
  return true;
}

bool MCP23017::InitPin(uint8_t pin, Mcp23017PinMode mode) {
  if (!m_isInitialized || !IsValidPin(pin)) {
    return false;
  }

  uint8_t* directionRegister = pin < 8 ? &m_directionA : &m_directionB;
  const uint8_t bit = pin % 8;

  if (mode == Mcp23017PinMode::Input) {
    *directionRegister |= (1 << bit);
  } else {
    *directionRegister &= ~(1 << bit);
  }

  return WriteRegister(pin < 8 ? RegisterIodirA : RegisterIodirB, *directionRegister);
}

bool MCP23017::SetPin(uint8_t pin, bool state) {
  if (!m_isInitialized || !IsValidPin(pin)) {
    return false;
  }

  uint8_t* outputRegister = pin < 8 ? &m_outputA : &m_outputB;
  const uint8_t bit = pin % 8;

  if (state) {
    *outputRegister |= (1 << bit);
  } else {
    *outputRegister &= ~(1 << bit);
  }

  return WriteRegister(pin < 8 ? RegisterOlatA : RegisterOlatB, *outputRegister);
}

bool MCP23017::SetPin(const Mcp23017Pin& pin) {
  if (!InitPin(pin.pin, pin.mode)) {
    return false;
  }

  if (pin.mode == Mcp23017PinMode::Output) {
    return SetPin(pin.pin, pin.state);
  }

  return true;
}

bool MCP23017::SetAllPins(const std::array<bool, 16>& states) {
  if (!m_isInitialized) {
    return false;
  }

  uint8_t outputA = 0x00;
  uint8_t outputB = 0x00;

  for (uint8_t i = 0; i < 8; i++) {
    if (states[i]) {
      outputA |= (1 << i);
    }

    if (states[i + 8]) {
      outputB |= (1 << i);
    }
  }

  if (!WriteRegister(RegisterOlatA, outputA)) {
    return false;
  }

  if (!WriteRegister(RegisterOlatB, outputB)) {
    return false;
  }

  m_outputA = outputA;
  m_outputB = outputB;

  return true;
}

bool MCP23017::GetPin(uint8_t pin, bool& state) {
  if (!m_isInitialized || !IsValidPin(pin)) {
    return false;
  }

  uint8_t value = 0;
  const uint8_t reg = pin < 8 ? RegisterGpioA : RegisterGpioB;

  if (!ReadRegister(reg, value)) {
    return false;
  }

  state = (value & (1 << (pin % 8))) != 0;
  return true;
}

bool MCP23017::GetAllPins(std::array<bool, 16>& states) {
  if (!m_isInitialized) {
    return false;
  }

  uint8_t gpioA = 0;
  uint8_t gpioB = 0;

  if (!ReadRegister(RegisterGpioA, gpioA)) {
    return false;
  }

  if (!ReadRegister(RegisterGpioB, gpioB)) {
    return false;
  }

  for (uint8_t i = 0; i < 8; i++) {
    states[i] = (gpioA & (1 << i)) != 0;
    states[i + 8] = (gpioB & (1 << i)) != 0;
  }

  return true;
}

bool MCP23017::SetPullUp(uint8_t pin, bool enabled) {
  if (!m_isInitialized || !IsValidPin(pin)) {
    return false;
  }

  uint8_t* pullUpRegister = pin < 8 ? &m_pullUpA : &m_pullUpB;
  const uint8_t bit = pin % 8;

  if (enabled) {
    *pullUpRegister |= (1 << bit);
  } else {
    *pullUpRegister &= ~(1 << bit);
  }

  return WriteRegister(pin < 8 ? RegisterGppuA : RegisterGppuB, *pullUpRegister);
}

bool MCP23017::WriteRegister(uint8_t reg, uint8_t value) {
  if (m_i2c == nullptr) {
    return false;
  }

  const uint8_t data[2] = {reg, value};
  return m_i2c->Write(m_address, data, sizeof(data));
}

bool MCP23017::ReadRegister(uint8_t reg, uint8_t& value) {
  if (m_i2c == nullptr) {
    return false;
  }

  return m_i2c->WriteRead(m_address, &reg, 1, &value, 1);
}

bool MCP23017::IsValidPin(uint8_t pin) const {
  return pin < 16;
}