#include "MCP23017.h"

class Mcp23017DigitalPin : public DigitalPin {
public:
  Mcp23017DigitalPin(MCP23017& parent, uint8_t pin)
    : m_parent(parent),
      m_pin(pin) {
  }

  bool Set(bool state) override {
    return m_parent.SetPin(m_pin, state);
  }

  bool Get(bool& state) override {
    return m_parent.GetPin(m_pin, state);
  }

  bool SetMode(bool isOutput) override {
    return m_parent.InitPin(
      m_pin,
      isOutput ? Mcp23017PinMode::Output : Mcp23017PinMode::Input
    );
  }

  bool SetPullUp(bool enabled) override {
    return m_parent.SetPullUp(m_pin, enabled);
  }

  bool AttachInterrupt(
    gpio_int_type_t interruptType,
    void (*callback)(void*),
    void* argument = nullptr
  ) override {
    return m_parent.AttachPinInterrupt(m_pin, interruptType, callback, argument);
  }

  bool DetachInterrupt() override {
    return m_parent.DetachPinInterrupt(m_pin);
  }

private:
  MCP23017& m_parent;
  uint8_t m_pin = 0;
};

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

  m_pullUpA = 0x00;
  m_pullUpB = 0x00;

  m_gpintenA = 0x00;
  m_gpintenB = 0x00;

  m_defvalA = 0x00;
  m_defvalB = 0x00;

  m_intconA = 0x00;
  m_intconB = 0x00;

  if (!WriteRegister(RegisterIoconA, 0x00)) {
    return false;
  }

  if (!WriteRegister(RegisterIoconB, 0x00)) {
    return false;
  }

  if (!WriteRegister(RegisterIodirA, m_directionA)) {
    return false;
  }

  if (!WriteRegister(RegisterIodirB, m_directionB)) {
    return false;
  }

  if (!WriteRegister(RegisterGppuA, m_pullUpA)) {
    return false;
  }

  if (!WriteRegister(RegisterGppuB, m_pullUpB)) {
    return false;
  }

  if (!WriteRegister(RegisterGpintenA, m_gpintenA)) {
    return false;
  }

  if (!WriteRegister(RegisterGpintenB, m_gpintenB)) {
    return false;
  }

  if (!WriteRegister(RegisterOlatA, m_outputA)) {
    return false;
  }

  if (!WriteRegister(RegisterOlatB, m_outputB)) {
    return false;
  }

  ReadRegister(RegisterGpioA, m_lastGpioA);
  ReadRegister(RegisterGpioB, m_lastGpioB);

  for (uint8_t i = 0; i < 16; i++) {
    m_pinObjects[i] = std::make_shared<Mcp23017DigitalPin>(*this, i);
    m_interruptCallbacks[i] = {};
  }

  m_isInitialized = true;
  return true;
}

std::shared_ptr<DigitalPin> MCP23017::GetPinObject(uint8_t pin) {
  if (!IsValidPin(pin)) {
    return nullptr;
  }

  if (m_pinObjects[pin] == nullptr) {
    m_pinObjects[pin] = std::make_shared<Mcp23017DigitalPin>(*this, pin);
  }

  return m_pinObjects[pin];
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

bool MCP23017::StartInterruptsWithEspPins(
  gpio_num_t intAPin,
  gpio_num_t intBPin,
  BaseType_t coreId,
  UBaseType_t priority
) {
  if (!m_isInitialized) {
    return false;
  }

  m_interruptMode = Mcp23017InterruptMode::EspGpioInterrupt;

  if (!WriteRegister(RegisterIoconA, 0x00)) {
    return false;
  }

  if (!WriteRegister(RegisterIoconB, 0x00)) {
    return false;
  }

  m_intAPin.SetPin(static_cast<uint8_t>(intAPin));
  m_intAPin.SetMode(GpioMode::Input);
  m_intAPin.SetPullMode(GpioPullMode::PullUp);

  m_intBPin.SetPin(static_cast<uint8_t>(intBPin));
  m_intBPin.SetMode(GpioMode::Input);
  m_intBPin.SetPullMode(GpioPullMode::PullUp);

  if (!StartWorker(coreId, priority)) {
    return false;
  }

  if (!m_intAPin.AttachInterrupt(GPIO_INTR_NEGEDGE, InterruptAStatic, this)) {
    return false;
  }

  if (!m_intBPin.AttachInterrupt(GPIO_INTR_NEGEDGE, InterruptBStatic, this)) {
    return false;
  }

  return true;
}

bool MCP23017::StartInterruptsWithPolling(
  uint32_t pollingIntervalMs,
  BaseType_t coreId,
  UBaseType_t priority
) {
  if (!m_isInitialized) {
    return false;
  }

  m_interruptMode = Mcp23017InterruptMode::Polling;
  m_pollingIntervalMs = pollingIntervalMs;

  ReadRegister(RegisterGpioA, m_lastGpioA);
  ReadRegister(RegisterGpioB, m_lastGpioB);

  return StartWorker(coreId, priority);
}

void MCP23017::StopInterrupts() {
  m_isWorkerRunning = false;

  m_intAPin.DetachInterrupt();
  m_intBPin.DetachInterrupt();

  m_interruptMode = Mcp23017InterruptMode::Disabled;
}

bool MCP23017::AttachPinInterrupt(
  uint8_t pin,
  gpio_int_type_t interruptType,
  void (*callback)(void*),
  void* argument
) {
  if (!m_isInitialized || !IsValidPin(pin) || callback == nullptr) {
    return false;
  }

  m_interruptCallbacks[pin].isEnabled = true;
  m_interruptCallbacks[pin].interruptType = interruptType;
  m_interruptCallbacks[pin].callback = callback;
  m_interruptCallbacks[pin].argument = argument;

  if (m_interruptMode == Mcp23017InterruptMode::EspGpioInterrupt) {
    return ConfigureHardwareInterrupt(pin, interruptType);
  }

  return true;
}

bool MCP23017::DetachPinInterrupt(uint8_t pin) {
  if (!m_isInitialized || !IsValidPin(pin)) {
    return false;
  }

  m_interruptCallbacks[pin] = {};

  uint8_t* gpintenRegister = pin < 8 ? &m_gpintenA : &m_gpintenB;
  const uint8_t bit = pin % 8;

  *gpintenRegister &= ~(1 << bit);

  return WriteRegister(pin < 8 ? RegisterGpintenA : RegisterGpintenB, *gpintenRegister);
}

bool MCP23017::ConfigureHardwareInterrupt(uint8_t pin, gpio_int_type_t interruptType) {
  uint8_t* gpintenRegister = pin < 8 ? &m_gpintenA : &m_gpintenB;
  uint8_t* intconRegister = pin < 8 ? &m_intconA : &m_intconB;
  uint8_t* defvalRegister = pin < 8 ? &m_defvalA : &m_defvalB;

  const uint8_t bit = pin % 8;
  const uint8_t mask = 1 << bit;

  *gpintenRegister |= mask;

  if (interruptType == GPIO_INTR_ANYEDGE) {
    *intconRegister &= ~mask;
  } else if (interruptType == GPIO_INTR_NEGEDGE) {
    *intconRegister |= mask;
    *defvalRegister |= mask;
  } else if (interruptType == GPIO_INTR_POSEDGE) {
    *intconRegister |= mask;
    *defvalRegister &= ~mask;
  } else {
    return false;
  }

  if (!WriteRegister(pin < 8 ? RegisterDefvalA : RegisterDefvalB, *defvalRegister)) {
    return false;
  }

  if (!WriteRegister(pin < 8 ? RegisterIntconA : RegisterIntconB, *intconRegister)) {
    return false;
  }

  if (!WriteRegister(pin < 8 ? RegisterGpintenA : RegisterGpintenB, *gpintenRegister)) {
    return false;
  }

  return true;
}

bool MCP23017::StartWorker(BaseType_t coreId, UBaseType_t priority) {
  if (m_workerHandle != nullptr) {
    return true;
  }

  if (m_interruptQueue == nullptr) {
    m_interruptQueue = xQueueCreate(16, sizeof(uint8_t));

    if (m_interruptQueue == nullptr) {
      return false;
    }
  }

  m_isWorkerRunning = true;

  return xTaskCreatePinnedToCore(
    WorkerTask,
    "Mcp23017Worker",
    4096,
    this,
    priority,
    &m_workerHandle,
    coreId
  ) == pdPASS;
}

void MCP23017::InterruptAStatic(void* argument) {
  MCP23017* expander = static_cast<MCP23017*>(argument);

  if (expander != nullptr) {
    expander->QueueInterruptPort(0);
  }
}

void MCP23017::InterruptBStatic(void* argument) {
  MCP23017* expander = static_cast<MCP23017*>(argument);

  if (expander != nullptr) {
    expander->QueueInterruptPort(1);
  }
}

void MCP23017::QueueInterruptPort(uint8_t port) {
  if (m_interruptQueue == nullptr) {
    return;
  }

  xQueueSend(m_interruptQueue, &port, 0);
}

void MCP23017::WorkerTask(void* argument) {
  MCP23017* expander = static_cast<MCP23017*>(argument);

  if (expander == nullptr) {
    vTaskDelete(nullptr);
    return;
  }

  while (expander->m_isWorkerRunning) {
    if (expander->m_interruptMode == Mcp23017InterruptMode::Polling) {
      expander->HandlePolling();
      vTaskDelay(pdMS_TO_TICKS(expander->m_pollingIntervalMs));
      continue;
    }

    uint8_t port = 0;

    if (xQueueReceive(expander->m_interruptQueue, &port, portMAX_DELAY) == pdTRUE) {
      expander->HandleHardwareInterrupt(port);
    }
  }

  expander->m_workerHandle = nullptr;
  vTaskDelete(nullptr);
}

void MCP23017::HandleHardwareInterrupt(uint8_t port) {
  uint8_t intf = 0;
  uint8_t intcap = 0;
  uint8_t gpio = 0;

  if (port == 0) {
    if (!ReadRegister(RegisterIntfA, intf)) {
      return;
    }

    if (!ReadRegister(RegisterIntcapA, intcap)) {
      return;
    }

    ReadRegister(RegisterGpioA, gpio);
    m_lastGpioA = gpio;
  } else {
    if (!ReadRegister(RegisterIntfB, intf)) {
      return;
    }

    if (!ReadRegister(RegisterIntcapB, intcap)) {
      return;
    }

    ReadRegister(RegisterGpioB, gpio);
    m_lastGpioB = gpio;
  }

  for (uint8_t bit = 0; bit < 8; bit++) {
    if ((intf & (1 << bit)) == 0) {
      continue;
    }

    const uint8_t pin = port == 0 ? bit : bit + 8;
    const bool capturedState = (intcap & (1 << bit)) != 0;
    const gpio_int_type_t type = m_interruptCallbacks[pin].interruptType;

    bool shouldRun = false;

    if (type == GPIO_INTR_ANYEDGE) {
      shouldRun = true;
    } else if (type == GPIO_INTR_NEGEDGE && !capturedState) {
      shouldRun = true;
    } else if (type == GPIO_INTR_POSEDGE && capturedState) {
      shouldRun = true;
    }

    if (shouldRun) {
      RunCallback(pin);
    }
  }
}

void MCP23017::HandlePolling() {
  uint8_t gpioA = 0;
  uint8_t gpioB = 0;

  if (!ReadRegister(RegisterGpioA, gpioA)) {
    return;
  }

  if (!ReadRegister(RegisterGpioB, gpioB)) {
    return;
  }

  const uint8_t changedA = gpioA ^ m_lastGpioA;
  const uint8_t changedB = gpioB ^ m_lastGpioB;

  for (uint8_t bit = 0; bit < 8; bit++) {
    if (changedA & (1 << bit)) {
      const bool oldState = (m_lastGpioA & (1 << bit)) != 0;
      const bool newState = (gpioA & (1 << bit)) != 0;

      if (CheckInterruptMatch(oldState, newState, m_interruptCallbacks[bit].interruptType)) {
        RunCallback(bit);
      }
    }

    if (changedB & (1 << bit)) {
      const uint8_t pin = bit + 8;

      const bool oldState = (m_lastGpioB & (1 << bit)) != 0;
      const bool newState = (gpioB & (1 << bit)) != 0;

      if (CheckInterruptMatch(oldState, newState, m_interruptCallbacks[pin].interruptType)) {
        RunCallback(pin);
      }
    }
  }

  m_lastGpioA = gpioA;
  m_lastGpioB = gpioB;
}

bool MCP23017::CheckInterruptMatch(
  bool oldState,
  bool newState,
  gpio_int_type_t interruptType
) const {
  if (oldState == newState) {
    return false;
  }

  if (interruptType == GPIO_INTR_ANYEDGE) {
    return true;
  }

  if (interruptType == GPIO_INTR_NEGEDGE) {
    return oldState && !newState;
  }

  if (interruptType == GPIO_INTR_POSEDGE) {
    return !oldState && newState;
  }

  return false;
}

void MCP23017::RunCallback(uint8_t pin) {
  if (!IsValidPin(pin)) {
    return;
  }

  Mcp23017InterruptCallback& interrupt = m_interruptCallbacks[pin];

  if (!interrupt.isEnabled || interrupt.callback == nullptr) {
    return;
  }

  interrupt.callback(interrupt.argument);
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