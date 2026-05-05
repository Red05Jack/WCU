#include "Window.h"

#include "esp_timer.h"

Window::Window() {
}

Window::Window(
  const WindowConfig& config,
  std::shared_ptr<DigitalPin> buttonOpen,
  std::shared_ptr<DigitalPin> buttonOpenAll,
  std::shared_ptr<DigitalPin> buttonClose,
  std::shared_ptr<DigitalPin> buttonCloseAll,
  std::shared_ptr<DigitalPin> motorA,
  std::shared_ptr<DigitalPin> motorB
) {
  Init(config, buttonOpen, buttonOpenAll, buttonClose, buttonCloseAll, motorA, motorB);
}

bool Window::Init(
  const WindowConfig& config,
  std::shared_ptr<DigitalPin> buttonOpen,
  std::shared_ptr<DigitalPin> buttonOpenAll,
  std::shared_ptr<DigitalPin> buttonClose,
  std::shared_ptr<DigitalPin> buttonCloseAll,
  std::shared_ptr<DigitalPin> motorA,
  std::shared_ptr<DigitalPin> motorB
) {
  if (
    config.state == nullptr ||
    buttonOpen == nullptr ||
    buttonOpenAll == nullptr ||
    buttonClose == nullptr ||
    buttonCloseAll == nullptr ||
    motorA == nullptr ||
    motorB == nullptr
  ) {
    return false;
  }

  m_config = config;

  m_buttonOpen = buttonOpen;
  m_buttonOpenAll = buttonOpenAll;
  m_buttonClose = buttonClose;
  m_buttonCloseAll = buttonCloseAll;

  m_motorA = motorA;
  m_motorB = motorB;

  m_buttonOpen->SetMode(false);
  m_buttonOpenAll->SetMode(false);
  m_buttonClose->SetMode(false);
  m_buttonCloseAll->SetMode(false);

  m_buttonOpen->SetPullUp(true);
  m_buttonOpenAll->SetPullUp(true);
  m_buttonClose->SetPullUp(true);
  m_buttonCloseAll->SetPullUp(true);

  m_motorA->SetMode(true);
  m_motorB->SetMode(true);

  SetMotors(false, false);

  m_isInitialized = true;
  return true;
}

bool Window::StartWindow() {
  if (!m_isInitialized) {
    return false;
  }

  m_isRunning = true;

  m_buttonOpen->AttachInterrupt(GPIO_INTR_ANYEDGE, ButtonOpenInterrupt, this);
  m_buttonOpenAll->AttachInterrupt(GPIO_INTR_NEGEDGE, ButtonOpenAllInterrupt, this);
  m_buttonClose->AttachInterrupt(GPIO_INTR_ANYEDGE, ButtonCloseInterrupt, this);
  m_buttonCloseAll->AttachInterrupt(GPIO_INTR_NEGEDGE, ButtonCloseAllInterrupt, this);

  if (m_workerHandle == nullptr) {
    return xTaskCreatePinnedToCore(
      WorkerTask,
      "WindowWorker",
      4096,
      this,
      8,
      &m_workerHandle,
      1
    ) == pdPASS;
  }

  return true;
}

void Window::StopWindow() {
  m_isRunning = false;

  m_buttonOpen->DetachInterrupt();
  m_buttonOpenAll->DetachInterrupt();
  m_buttonClose->DetachInterrupt();
  m_buttonCloseAll->DetachInterrupt();

  Stop();
}

void Window::StartOpen() {
  StartMove(WindowDirection::Open, false);
}

void Window::StartClose() {
  StartMove(WindowDirection::Close, false);
}

void Window::OpenAll() {
  StartMove(WindowDirection::Open, true);
}

void Window::CloseAll() {
  StartMove(WindowDirection::Close, true);
}

void Window::Toggle() {
  if (GetCurrentState() < 128) {
    OpenAll();
  } else {
    CloseAll();
  }
}

void Window::Stop() {
  StopInternal(true);
}

uint32_t Window::GetTimeOpen() {
  uint8_t high = m_config.state->GetState(m_config.timeOpenHighPos);
  uint8_t low = m_config.state->GetState(m_config.timeOpenLowPos);

  return ((((uint16_t)high << 8) | low) * 10);
}

uint32_t Window::GetTimeClose() {
  uint8_t high = m_config.state->GetState(m_config.timeCloseHighPos);
  uint8_t low = m_config.state->GetState(m_config.timeCloseLowPos);

  return ((((uint16_t)high << 8) | low) * 10);
}

uint8_t Window::GetCurrentState() {
  return m_config.state->GetState(m_config.statePos);
}

bool Window::SetCurrentState(uint8_t state) {
  return m_config.state->SetState(state, m_config.statePos);
}

bool Window::CalibrateWindows() {
  const bool isOpenSet = SetTimeOpen(5000);
  const bool isCloseSet = SetTimeClose(5000);

  SetCurrentState(0);

  return isOpenSet && isCloseSet;
}

bool Window::SetTimeOpen(uint32_t timeMs) {
  if (timeMs > 655350) {
    timeMs = 655350;
  }

  uint16_t raw = timeMs / 10;

  uint8_t high = (raw >> 8) & 0xFF;
  uint8_t low = raw & 0xFF;

  bool isHighSet = m_config.state->SetState(high, m_config.timeOpenHighPos);
  bool isLowSet = m_config.state->SetState(low, m_config.timeOpenLowPos);

  return isHighSet && isLowSet;
}

bool Window::SetTimeClose(uint32_t timeMs) {
  if (timeMs > 655350) {
    timeMs = 655350;
  }

  uint16_t raw = timeMs / 10;

  uint8_t high = (raw >> 8) & 0xFF;
  uint8_t low = raw & 0xFF;

  bool isHighSet = m_config.state->SetState(high, m_config.timeCloseHighPos);
  bool isLowSet = m_config.state->SetState(low, m_config.timeCloseLowPos);

  return isHighSet && isLowSet;
}

uint32_t Window::GetRemainingOpenTime() {
  const uint32_t fullTime = GetTimeOpen();
  const uint8_t currentState = GetCurrentState();

  return ((255 - currentState) * fullTime) / 255;
}

uint32_t Window::GetRemainingCloseTime() {
  const uint32_t fullTime = GetTimeClose();
  const uint8_t currentState = GetCurrentState();

  return (currentState * fullTime) / 255;
}

void Window::StartMove(WindowDirection direction, bool moveAll) {
  if (!m_isInitialized || direction == WindowDirection::Stop) {
    return;
  }

  StopInternal(true);

  m_startState = GetCurrentState();
  m_direction = direction;
  m_moveStartUs = esp_timer_get_time();

  if (direction == WindowDirection::Open) {
    m_moveDurationMs = moveAll ? GetRemainingOpenTime() : GetRemainingOpenTime();
    SetMotors(true, false);
  } else {
    m_moveDurationMs = moveAll ? GetRemainingCloseTime() : GetRemainingCloseTime();
    SetMotors(false, true);
  }

  if (m_moveDurationMs == 0) {
    StopInternal(true);
  }
}

void Window::StopInternal(bool savePosition) {
  if (savePosition) {
    UpdateStateFromElapsedTime();
  }

  SetMotors(false, false);

  m_direction = WindowDirection::Stop;
  m_moveDurationMs = 0;
  m_moveStartUs = 0;
}

void Window::UpdateStateFromElapsedTime() {
  if (m_direction == WindowDirection::Stop || m_moveStartUs == 0 || m_moveDurationMs == 0) {
    return;
  }

  const uint64_t nowUs = esp_timer_get_time();
  uint32_t elapsedMs = (nowUs - m_moveStartUs) / 1000;

  if (elapsedMs > m_moveDurationMs) {
    elapsedMs = m_moveDurationMs;
  }

  uint8_t newState = m_startState;

  if (m_direction == WindowDirection::Open) {
    const uint32_t fullTime = GetTimeOpen();

    if (fullTime > 0) {
      uint32_t delta = (elapsedMs * 255) / fullTime;
      uint32_t value = m_startState + delta;
      newState = value > 255 ? 255 : value;
    }
  }

  if (m_direction == WindowDirection::Close) {
    const uint32_t fullTime = GetTimeClose();

    if (fullTime > 0) {
      uint32_t delta = (elapsedMs * 255) / fullTime;
      newState = delta > m_startState ? 0 : m_startState - delta;
    }
  }

  SetCurrentState(newState);
}

void Window::SetMotors(bool motorAState, bool motorBState) {
  if (motorAState && motorBState) {
    motorAState = false;
    motorBState = false;
  }

  m_motorA->Set(motorAState);
  m_motorB->Set(motorBState);
}

void Window::HandleButtonOpen() {
  bool raw = true;
  m_buttonOpen->Get(raw);

  if (!raw) {
    StartOpen();
  } else if (m_direction == WindowDirection::Open) {
    Stop();
  }
}

void Window::HandleButtonOpenAll() {
  if (m_direction == WindowDirection::Open) {
    Stop();
  } else {
    OpenAll();
  }
}

void Window::HandleButtonClose() {
  bool raw = true;
  m_buttonClose->Get(raw);

  if (!raw) {
    StartClose();
  } else if (m_direction == WindowDirection::Close) {
    Stop();
  }
}

void Window::HandleButtonCloseAll() {
  if (m_direction == WindowDirection::Close) {
    Stop();
  } else {
    CloseAll();
  }
}

void Window::ButtonOpenInterrupt(void* argument) {
  Window* window = static_cast<Window*>(argument);

  if (window != nullptr) {
    window->HandleButtonOpen();
  }
}

void Window::ButtonOpenAllInterrupt(void* argument) {
  Window* window = static_cast<Window*>(argument);

  if (window != nullptr) {
    window->HandleButtonOpenAll();
  }
}

void Window::ButtonCloseInterrupt(void* argument) {
  Window* window = static_cast<Window*>(argument);

  if (window != nullptr) {
    window->HandleButtonClose();
  }
}

void Window::ButtonCloseAllInterrupt(void* argument) {
  Window* window = static_cast<Window*>(argument);

  if (window != nullptr) {
    window->HandleButtonCloseAll();
  }
}

void Window::WorkerTask(void* argument) {
  Window* window = static_cast<Window*>(argument);

  while (window != nullptr && window->m_isRunning) {
    if (
      window->m_direction != WindowDirection::Stop &&
      window->m_moveStartUs != 0 &&
      window->m_moveDurationMs > 0
    ) {
      const uint64_t nowUs = esp_timer_get_time();
      const uint32_t elapsedMs = (nowUs - window->m_moveStartUs) / 1000;

      if (elapsedMs >= window->m_moveDurationMs) {
        window->Stop();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if (window != nullptr) {
    window->m_workerHandle = nullptr;
  }

  vTaskDelete(nullptr);
}