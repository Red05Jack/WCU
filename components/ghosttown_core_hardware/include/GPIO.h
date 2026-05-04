// GPIO.h
#pragma once

#include <cstdint>
#include <vector>

#include "driver/gpio.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

enum class GpioMode : uint8_t {
  Input,
  Output,
  InputOutput
};

enum class GpioPullMode : int8_t {
  PullDown = -1,
  None = 0,
  PullUp = 1
};

struct GpioJob {
  uint8_t pin = 0;
  uint64_t timestampUs = 0;
  bool state = false;
  bool (*condition)() = nullptr;
  void* owner = nullptr;
};

struct GpioInterruptJob {
  uint8_t pin = 0;
  void* owner = nullptr;
};

class GPIO {
public:
  GPIO();
  GPIO(uint8_t pin, GpioMode mode, GpioPullMode pullMode = GpioPullMode::None, bool hasInterrupts = false);
  ~GPIO();

  bool SetPin(uint8_t pin);
  bool SetMode(GpioMode mode);
  bool SetPullMode(GpioPullMode pullMode);
  bool SetInterrupts(bool hasInterrupts);

  bool Set(bool state);
  bool Set(bool state, uint64_t timestampUs);
  bool Set(bool state, bool (*condition)());
  bool Set(bool state, uint64_t timestampUs, bool (*condition)());

  int Get() const;

  bool ClearQueueEntries();
  static bool ClearAllQueueEntries();

  static bool StartWorker(BaseType_t coreId = 1, UBaseType_t priority = 10);
  static void StopWorker();

  bool AttachInterrupt(gpio_int_type_t interruptType, void (*callback)(void*), void* argument = nullptr);
  bool DetachInterrupt();

private:
  static void IRAM_ATTR InterruptHandler(void* argument);
  static void InterruptWorkerTask(void* parameter);

private:
  void (*m_interruptCallback)(void*) = nullptr;
  void* m_interruptArgument = nullptr;
  bool m_hasAttachedInterrupt = false;

  static QueueHandle_t m_interruptQueue;
  static TaskHandle_t m_interruptWorkerHandle;
  static bool m_isInterruptWorkerRunning;

private:
  static void WorkerTask(void* parameter);
  static bool PushJob(const GpioJob& job);
  static bool IsJobReady(const GpioJob& job, uint64_t nowUs);

  bool SaveConfig();

private:
  uint8_t m_pin = 255;
  GpioMode m_mode = GpioMode::Input;
  GpioPullMode m_pullMode = GpioPullMode::None;
  bool m_hasInterrupts = false;

  gpio_config_t m_config = {};

  static std::vector<GpioJob> m_jobs;
  static SemaphoreHandle_t m_mutex;
  static TaskHandle_t m_workerHandle;
  static bool m_isWorkerRunning;
};