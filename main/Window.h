#pragma once

#include <cstdint>
#include <memory>

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

class Window {
public:
  Window();

  Window(
    const WindowConfig& config,
    std::shared_ptr<DigitalPin> buttonOpen,
    std::shared_ptr<DigitalPin> buttonOpenAll,
    std::shared_ptr<DigitalPin> buttonClose,
    std::shared_ptr<DigitalPin> buttonCloseAll,
    std::shared_ptr<DigitalPin> motorA,
    std::shared_ptr<DigitalPin> motorB
  );

  bool Init(
    const WindowConfig& config,
    std::shared_ptr<DigitalPin> buttonOpen,
    std::shared_ptr<DigitalPin> buttonOpenAll,
    std::shared_ptr<DigitalPin> buttonClose,
    std::shared_ptr<DigitalPin> buttonCloseAll,
    std::shared_ptr<DigitalPin> motorA,
    std::shared_ptr<DigitalPin> motorB
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

private:
  static void ButtonOpenInterrupt(void* argument);
  static void ButtonOpenAllInterrupt(void* argument);
  static void ButtonCloseInterrupt(void* argument);
  static void ButtonCloseAllInterrupt(void* argument);

  static void WorkerTask(void* argument);

  void HandleButtonOpen();
  void HandleButtonOpenAll();
  void HandleButtonClose();
  void HandleButtonCloseAll();

  void StartMove(WindowDirection direction, bool moveAll);
  void StopInternal(bool savePosition);

  bool SetTimeOpen(uint32_t timeMs);
  bool SetTimeClose(uint32_t timeMs);

  uint32_t GetRemainingOpenTime();
  uint32_t GetRemainingCloseTime();

  void UpdateStateFromElapsedTime();
  void SetMotors(bool motorAState, bool motorBState);

private:
  WindowConfig m_config = {};

  std::shared_ptr<DigitalPin> m_buttonOpen;
  std::shared_ptr<DigitalPin> m_buttonOpenAll;
  std::shared_ptr<DigitalPin> m_buttonClose;
  std::shared_ptr<DigitalPin> m_buttonCloseAll;

  std::shared_ptr<DigitalPin> m_motorA;
  std::shared_ptr<DigitalPin> m_motorB;

  TaskHandle_t m_workerHandle = nullptr;

  bool m_isRunning = false;
  bool m_isInitialized = false;

  WindowDirection m_direction = WindowDirection::Stop;

  uint64_t m_moveStartUs = 0;
  uint32_t m_moveDurationMs = 0;
  uint8_t m_startState = 0;
};