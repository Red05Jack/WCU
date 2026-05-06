#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "DigitalPin.h"
#include "State.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct WindowConfig {
  uint8_t statePos = 0;

  uint8_t timeOpenHighPos = 1;
  uint8_t timeOpenLowPos = 2;
  uint8_t timeCloseHighPos = 3;
  uint8_t timeCloseLowPos = 4;

  State* state = nullptr;
};

enum class WindowDirection : uint8_t {
  Stop,
  Open,
  Close
};

enum class WindowAction : uint8_t {
  Open,
  OpenAll,
  Close,
  CloseAll,
  Toggle
};

struct WindowInput {
  WindowAction action = WindowAction::Toggle;
  std::shared_ptr<DigitalPin> button = nullptr;

  std::shared_ptr<DigitalPin> powerWindowLock = nullptr;

  bool buttonActiveState = false;      // Pull-up button: pressed = false
  bool powerLockActiveState = true;    // Lock active when pin reads true
  bool lastButtonState = true;
};

class Window {
public:
  Window();

  Window(
    const WindowConfig& config,
    std::shared_ptr<DigitalPin> motorOpen,
    std::shared_ptr<DigitalPin> motorClose
  );

  bool Init(
    const WindowConfig& config,
    std::shared_ptr<DigitalPin> motorOpen,
    std::shared_ptr<DigitalPin> motorClose
  );

  bool AddInput(
    WindowAction action,
    std::shared_ptr<DigitalPin> button,
    std::shared_ptr<DigitalPin> powerWindowLock = nullptr,
    bool buttonActiveState = false,
    bool powerLockActiveState = true
  );

  bool StartWindow();
  void StopWindow();

  void StartOpen();
  void StartClose();

  void Stop();

  void OpenAll();
  void CloseAll();

  void Toggle();

  uint32_t GetTimeOpen();
  uint32_t GetTimeClose();

  uint8_t GetCurrentState();
  bool SetCurrentState(uint8_t state);

  bool CalibrateWindows();

  bool IsMoving() const;
  WindowDirection GetDirection() const;

private:
  struct CallbackContext {
    Window* window = nullptr;
    size_t inputIndex = 0;
  };

  static void InputInterrupt(void* argument);
  static void WorkerTask(void* argument);

  void HandleInput(size_t inputIndex);
  bool IsInputLocked(const WindowInput& input);
  bool IsInputPressed(WindowInput& input);

  void ExecuteAction(WindowAction action, bool isPressed, bool wasPressed);

  void StartMove(WindowDirection direction, bool moveAll);
  void StopInternal(bool savePosition);

  bool SetTimeOpen(uint32_t timeMs);
  bool SetTimeClose(uint32_t timeMs);

  uint32_t GetRemainingOpenTime();
  uint32_t GetRemainingCloseTime();

  void UpdateStateFromElapsedTime();
  void SetMotors(bool motorOpenState, bool motorCloseState);

private:
  WindowConfig m_config = {};

  std::shared_ptr<DigitalPin> m_motorOpen;
  std::shared_ptr<DigitalPin> m_motorClose;

  std::vector<WindowInput> m_inputs;
  std::vector<CallbackContext> m_callbackContexts;

  TaskHandle_t m_workerHandle = nullptr;

  bool m_isRunning = false;
  bool m_isInitialized = false;

  WindowDirection m_direction = WindowDirection::Stop;

  uint64_t m_moveStartUs = 0;
  uint32_t m_moveDurationMs = 0;
  uint8_t m_startState = 0;
};