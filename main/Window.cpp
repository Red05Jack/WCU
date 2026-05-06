#include "Window.h"

#include "esp_timer.h"

Window::Window() {
}

Window::Window(
  const WindowConfig& config,
  std::shared_ptr<DigitalPin> motorOpen,
  std::shared_ptr<DigitalPin> motorClose
) {
  Init(config, motorOpen, motorClose);
}

bool Window::Init(
  const WindowConfig& config,
  std::shared_ptr<DigitalPin> motorOpen,
  std::shared_ptr<DigitalPin> motorClose
) {
  if (config.state == nullptr || motorOpen == nullptr || motorClose == nullptr) {
    return false;
  }

  m_config = config;
  m_motorOpen = motorOpen;
  m_motorClose = motorClose;

  m_motorOpen->SetMode(true);
  m_motorClose->SetMode(true);

  SetMotors(false, false);

  m_isInitialized = true;
  return true;
}

bool Window::AddInput(
  WindowAction action,
  std::shared_ptr<DigitalPin> button,
  std::shared_ptr<DigitalPin> powerWindowLock,
  bool buttonActiveState,
  bool powerLockActiveState
) {
  if (button == nullptr) {
    return false;
  }

  WindowInput input;
  input.action = action;
  input.button = button;
  input.powerWindowLock = powerWindowLock;
  input.buttonActiveState = buttonActiveState;
  input.powerLockActiveState = powerLockActiveState;

  input.button->SetMode(false);
  input.button->SetPullUp(true);

  if (input.powerWindowLock != nullptr) {
    input.powerWindowLock->SetMode(false);
    input.powerWindowLock->SetPullUp(true);
  }

  input.button->Get(input.lastButtonState);

  m_inputs.push_back(input);

  CallbackContext context;
  context.window = this;
  context.inputIndex = m_inputs.size() - 1;

  m_callbackContexts.push_back(context);

  return true;
}

bool Window::StartWindow() {
  if (!m_isInitialized) {
    return false;
  }

  m_isRunning = true;

  for (size_t i = 0; i < m_inputs.size(); i++) {
    m_inputs[i].button->AttachInterrupt(
      GPIO_INTR_ANYEDGE,
      InputInterrupt,
      &m_callbackContexts[i]
    );
  }

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

  for (WindowInput& input : m_inputs) {
    if (input.button != nullptr) {
      input.button->DetachInterrupt();
    }
  }

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
  if (m_direction != WindowDirection::Stop) {
    Stop();
    return;
  }

  if (GetCurrentState() < 128) {
    OpenAll();
  } else {
    CloseAll();
  }
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

bool Window::IsMoving() const {
  return m_direction != WindowDirection::Stop;
}

WindowDirection Window::GetDirection() const {
  return m_direction;
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

void Window::Stop() {
  StopInternal(true);
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

void Window::SetMotors(bool motorOpenState, bool motorCloseState) {
  if (motorOpenState && motorCloseState) {
    motorOpenState = false;
    motorCloseState = false;
  }

  m_motorOpen->Set(motorOpenState);
  m_motorClose->Set(motorCloseState);
}

bool Window::IsInputLocked(const WindowInput& input) {
  if (input.powerWindowLock == nullptr) {
    return false;
  }

  bool lockState = false;

  if (!input.powerWindowLock->Get(lockState)) {
    return true;
  }

  return lockState == input.powerLockActiveState;
}

bool Window::IsInputPressed(WindowInput& input) {
  bool state = true;

  if (!input.button->Get(state)) {
    return false;
  }

  return state == input.buttonActiveState;
}

void Window::ExecuteAction(WindowAction action, bool isPressed, bool wasPressed) {
  switch (action) {
    case WindowAction::Open:
      if (isPressed && !wasPressed) {
        StartOpen();
      }

      if (!isPressed && wasPressed && m_direction == WindowDirection::Open) {
        Stop();
      }
      break;

    case WindowAction::Close:
      if (isPressed && !wasPressed) {
        StartClose();
      }

      if (!isPressed && wasPressed && m_direction == WindowDirection::Close) {
        Stop();
      }
      break;

    case WindowAction::OpenAll:
      if (isPressed && !wasPressed) {
        if (m_direction == WindowDirection::Open) {
          Stop();
        } else {
          OpenAll();
        }
      }
      break;

    case WindowAction::CloseAll:
      if (isPressed && !wasPressed) {
        if (m_direction == WindowDirection::Close) {
          Stop();
        } else {
          CloseAll();
        }
      }
      break;

    case WindowAction::Toggle:
      if (isPressed && !wasPressed) {
        Toggle();
      }
      break;
  }
}

void Window::HandleInput(size_t inputIndex) {
  if (inputIndex >= m_inputs.size()) {
    return;
  }

  WindowInput& input = m_inputs[inputIndex];

  const bool wasPressed = input.lastButtonState == input.buttonActiveState;
  const bool isPressed = IsInputPressed(input);

  input.lastButtonState = isPressed ? input.buttonActiveState : !input.buttonActiveState;

  if (IsInputLocked(input)) {
    return;
  }

  ExecuteAction(input.action, isPressed, wasPressed);
}

void Window::InputInterrupt(void* argument) {
  CallbackContext* context = static_cast<CallbackContext*>(argument);

  if (context == nullptr || context->window == nullptr) {
    return;
  }

  context->window->HandleInput(context->inputIndex);
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