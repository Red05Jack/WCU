#pragma once

#include <cstdint>
#include <array>
#include <memory>

#include "I2C.h"
#include "GPIO.h"
#include "DigitalPin.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

enum class Mcp23017PinMode : uint8_t {
  Output = 0,
  Input = 1
};

enum class Mcp23017InterruptMode : uint8_t {
  Disabled,
  EspGpioInterrupt,
  Polling
};

struct Mcp23017Pin {
  uint8_t pin = 0;
  Mcp23017PinMode mode = Mcp23017PinMode::Input;
  bool state = false;
};

struct Mcp23017InterruptCallback {
  bool isEnabled = false;
  gpio_int_type_t interruptType = GPIO_INTR_DISABLE;
  void (*callback)(void*) = nullptr;
  void* argument = nullptr;
};

class Mcp23017DigitalPin;

class MCP23017 {
public:
  MCP23017();
  MCP23017(I2C* i2c, uint8_t address = 0x20);

  bool Init(I2C* i2c, uint8_t address = 0x20);

  std::shared_ptr<DigitalPin> GetPinObject(uint8_t pin);

  bool InitPin(uint8_t pin, Mcp23017PinMode mode);

  bool SetPin(uint8_t pin, bool state);
  bool SetPin(const Mcp23017Pin& pin);

  bool SetAllPins(const std::array<bool, 16>& states);

  bool GetPin(uint8_t pin, bool& state);
  bool GetAllPins(std::array<bool, 16>& states);

  bool SetPullUp(uint8_t pin, bool enabled);

  bool StartInterruptsWithEspPins(
    gpio_num_t intAPin,
    gpio_num_t intBPin,
    BaseType_t coreId = 1,
    UBaseType_t priority = 10
  );

  bool StartInterruptsWithPolling(
    uint32_t pollingIntervalMs = 20,
    BaseType_t coreId = 1,
    UBaseType_t priority = 10
  );

  void StopInterrupts();

  bool AttachPinInterrupt(
    uint8_t pin,
    gpio_int_type_t interruptType,
    void (*callback)(void*),
    void* argument = nullptr
  );

  bool DetachPinInterrupt(uint8_t pin);

private:
  static constexpr uint8_t RegisterIodirA = 0x00;
  static constexpr uint8_t RegisterIodirB = 0x01;

  static constexpr uint8_t RegisterIpolA = 0x02;
  static constexpr uint8_t RegisterIpolB = 0x03;

  static constexpr uint8_t RegisterGpintenA = 0x04;
  static constexpr uint8_t RegisterGpintenB = 0x05;

  static constexpr uint8_t RegisterDefvalA = 0x06;
  static constexpr uint8_t RegisterDefvalB = 0x07;

  static constexpr uint8_t RegisterIntconA = 0x08;
  static constexpr uint8_t RegisterIntconB = 0x09;

  static constexpr uint8_t RegisterIoconA = 0x0A;
  static constexpr uint8_t RegisterIoconB = 0x0B;

  static constexpr uint8_t RegisterGppuA = 0x0C;
  static constexpr uint8_t RegisterGppuB = 0x0D;

  static constexpr uint8_t RegisterIntfA = 0x0E;
  static constexpr uint8_t RegisterIntfB = 0x0F;

  static constexpr uint8_t RegisterIntcapA = 0x10;
  static constexpr uint8_t RegisterIntcapB = 0x11;

  static constexpr uint8_t RegisterGpioA = 0x12;
  static constexpr uint8_t RegisterGpioB = 0x13;

  static constexpr uint8_t RegisterOlatA = 0x14;
  static constexpr uint8_t RegisterOlatB = 0x15;

  static void InterruptAStatic(void* argument);
  static void InterruptBStatic(void* argument);
  static void WorkerTask(void* argument);

  bool StartWorker(BaseType_t coreId, UBaseType_t priority);

  void QueueInterruptPort(uint8_t port);
  void HandleHardwareInterrupt(uint8_t port);
  void HandlePolling();

  bool ConfigureHardwareInterrupt(uint8_t pin, gpio_int_type_t interruptType);
  bool CheckInterruptMatch(bool oldState, bool newState, gpio_int_type_t interruptType) const;
  void RunCallback(uint8_t pin);

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

  uint8_t m_pullUpA = 0x00;
  uint8_t m_pullUpB = 0x00;

  uint8_t m_gpintenA = 0x00;
  uint8_t m_gpintenB = 0x00;

  uint8_t m_defvalA = 0x00;
  uint8_t m_defvalB = 0x00;

  uint8_t m_intconA = 0x00;
  uint8_t m_intconB = 0x00;

  uint8_t m_lastGpioA = 0x00;
  uint8_t m_lastGpioB = 0x00;

  bool m_isInitialized = false;

  Mcp23017InterruptMode m_interruptMode = Mcp23017InterruptMode::Disabled;
  uint32_t m_pollingIntervalMs = 20;

  GPIO m_intAPin;
  GPIO m_intBPin;

  QueueHandle_t m_interruptQueue = nullptr;
  TaskHandle_t m_workerHandle = nullptr;
  bool m_isWorkerRunning = false;

  std::array<std::shared_ptr<Mcp23017DigitalPin>, 16> m_pinObjects;
  std::array<Mcp23017InterruptCallback, 16> m_interruptCallbacks;
};