#include "GPIO.h"

#include <algorithm>

class GpioDigitalPin : public DigitalPin {
public:
  explicit GpioDigitalPin(GPIO& parent)
    : m_parent(parent) {
  }

  bool Set(bool state) override {
    return m_parent.Set(state);
  }

  bool Get(bool& state) override {
    const int value = m_parent.Get();

    if (value < 0) {
      return false;
    }

    state = value != 0;
    return true;
  }

  bool SetMode(bool isOutput) override {
    return m_parent.SetMode(isOutput ? GpioMode::Output : GpioMode::Input);
  }

  bool SetPullUp(bool enabled) override {
    return m_parent.SetPullMode(enabled ? GpioPullMode::PullUp : GpioPullMode::None);
  }

  bool AttachInterrupt(gpio_int_type_t interruptType, void (*callback)(void*), void* argument = nullptr) override {
    return m_parent.AttachInterrupt(interruptType, callback, argument);
  }

  bool DetachInterrupt() override {
    return m_parent.DetachInterrupt();
  }

private:
  GPIO& m_parent;
};

std::vector<GpioJob> GPIO::m_jobs;
SemaphoreHandle_t GPIO::m_mutex = nullptr;
TaskHandle_t GPIO::m_workerHandle = nullptr;
bool GPIO::m_isWorkerRunning = false;

QueueHandle_t GPIO::m_interruptQueue = nullptr;
TaskHandle_t GPIO::m_interruptWorkerHandle = nullptr;
bool GPIO::m_isInterruptWorkerRunning = false;

GPIO::GPIO() {
  if (m_mutex == nullptr) {
    m_mutex = xSemaphoreCreateMutex();
  }

  m_pinObject = std::make_shared<GpioDigitalPin>(*this);
}

GPIO::GPIO(uint8_t pin, GpioMode mode, GpioPullMode pullMode, bool hasInterrupts)
  : GPIO() {
  m_pin = pin;
  m_mode = mode;
  m_pullMode = pullMode;
  m_hasInterrupts = hasInterrupts;
  SaveConfig();
}

GPIO::~GPIO() {
  DetachInterrupt();
  ClearQueueEntries();
}

std::shared_ptr<DigitalPin> GPIO::GetPinObject() {
  return m_pinObject;
}

bool GPIO::SetPin(uint8_t pin) {
  m_pin = pin;
  return SaveConfig();
}

bool GPIO::SetMode(GpioMode mode) {
  m_mode = mode;
  return SaveConfig();
}

bool GPIO::SetPullMode(GpioPullMode pullMode) {
  m_pullMode = pullMode;
  return SaveConfig();
}

bool GPIO::SetInterrupts(bool hasInterrupts) {
  m_hasInterrupts = hasInterrupts;
  return SaveConfig();
}

bool GPIO::Set(bool state) {
  if (m_pin == 255) {
    return false;
  }

  return gpio_set_level(static_cast<gpio_num_t>(m_pin), state ? 1 : 0) == ESP_OK;
}

bool GPIO::Set(bool state, uint64_t timestampUs) {
  GpioJob job;
  job.pin = m_pin;
  job.timestampUs = timestampUs;
  job.state = state;
  job.condition = nullptr;
  job.owner = this;

  return PushJob(job);
}

bool GPIO::Set(bool state, bool (*condition)()) {
  GpioJob job;
  job.pin = m_pin;
  job.timestampUs = 0;
  job.state = state;
  job.condition = condition;
  job.owner = this;

  return PushJob(job);
}

bool GPIO::Set(bool state, uint64_t timestampUs, bool (*condition)()) {
  GpioJob job;
  job.pin = m_pin;
  job.timestampUs = timestampUs;
  job.state = state;
  job.condition = condition;
  job.owner = this;

  return PushJob(job);
}

int GPIO::Get() const {
  if (m_pin == 255) {
    return -1;
  }

  return gpio_get_level(static_cast<gpio_num_t>(m_pin));
}

bool GPIO::SaveConfig() {
  if (m_pin == 255) {
    return false;
  }

  m_config = {};
  m_config.pin_bit_mask = 1ULL << m_pin;

  switch (m_mode) {
    case GpioMode::Input:
      m_config.mode = GPIO_MODE_INPUT;
      break;

    case GpioMode::Output:
      m_config.mode = GPIO_MODE_OUTPUT;
      break;

    case GpioMode::InputOutput:
      m_config.mode = GPIO_MODE_INPUT_OUTPUT;
      break;
  }

  m_config.pull_down_en =
    m_pullMode == GpioPullMode::PullDown ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;

  m_config.pull_up_en =
    m_pullMode == GpioPullMode::PullUp ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;

  m_config.intr_type =
    m_hasInterrupts ? GPIO_INTR_ANYEDGE : GPIO_INTR_DISABLE;

  return gpio_config(&m_config) == ESP_OK;
}

bool GPIO::AttachInterrupt(gpio_int_type_t interruptType, void (*callback)(void*), void* argument) {
  if (m_pin == 255 || callback == nullptr) {
    return false;
  }

  m_interruptCallback = callback;
  m_interruptArgument = argument;

  if (gpio_set_intr_type(static_cast<gpio_num_t>(m_pin), interruptType) != ESP_OK) {
    return false;
  }

  if (m_interruptQueue == nullptr) {
    m_interruptQueue = xQueueCreate(32, sizeof(GpioInterruptJob));

    if (m_interruptQueue == nullptr) {
      return false;
    }
  }

  if (m_interruptWorkerHandle == nullptr) {
    m_isInterruptWorkerRunning = true;

    if (xTaskCreatePinnedToCore(
          InterruptWorkerTask,
          "GpioInterruptWorker",
          4096,
          nullptr,
          12,
          &m_interruptWorkerHandle,
          1
        ) != pdPASS) {
      m_isInterruptWorkerRunning = false;
      return false;
    }
  }

  esp_err_t result = gpio_install_isr_service(0);

  if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
    return false;
  }

  result = gpio_isr_handler_add(
    static_cast<gpio_num_t>(m_pin),
    InterruptHandler,
    this
  );

  if (result != ESP_OK) {
    return false;
  }

  m_hasAttachedInterrupt = true;
  return true;
}

bool GPIO::DetachInterrupt() {
  if (m_pin == 255 || !m_hasAttachedInterrupt) {
    return false;
  }

  gpio_isr_handler_remove(static_cast<gpio_num_t>(m_pin));
  m_hasAttachedInterrupt = false;

  return true;
}

void IRAM_ATTR GPIO::InterruptHandler(void* argument) {
  GPIO* gpio = static_cast<GPIO*>(argument);

  if (gpio == nullptr || m_interruptQueue == nullptr) {
    return;
  }

  GpioInterruptJob job;
  job.pin = gpio->m_pin;
  job.owner = gpio;

  BaseType_t hasHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(m_interruptQueue, &job, &hasHigherPriorityTaskWoken);

  if (hasHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

void GPIO::InterruptWorkerTask(void* parameter) {
  GpioInterruptJob job;

  while (m_isInterruptWorkerRunning) {
    if (xQueueReceive(m_interruptQueue, &job, portMAX_DELAY) == pdTRUE) {
      GPIO* gpio = static_cast<GPIO*>(job.owner);

      if (gpio != nullptr && gpio->m_interruptCallback != nullptr) {
        gpio->m_interruptCallback(gpio->m_interruptArgument);
      }
    }
  }

  m_interruptWorkerHandle = nullptr;
  vTaskDelete(nullptr);
}

bool GPIO::PushJob(const GpioJob& job) {
  if (m_mutex == nullptr || job.pin == 255) {
    return false;
  }

  xSemaphoreTake(m_mutex, portMAX_DELAY);
  m_jobs.push_back(job);
  xSemaphoreGive(m_mutex);

  return true;
}

bool GPIO::IsJobReady(const GpioJob& job, uint64_t nowUs) {
  const bool isTimeReady = job.timestampUs == 0 || nowUs >= job.timestampUs;
  const bool isConditionReady = job.condition == nullptr || job.condition() == false;

  return isTimeReady && isConditionReady;
}

void GPIO::WorkerTask(void* parameter) {
  while (m_isWorkerRunning) {
    const uint64_t nowUs = esp_timer_get_time();

    xSemaphoreTake(m_mutex, portMAX_DELAY);

    for (auto it = m_jobs.begin(); it != m_jobs.end();) {
      if (IsJobReady(*it, nowUs)) {
        gpio_set_level(static_cast<gpio_num_t>(it->pin), it->state ? 1 : 0);
        it = m_jobs.erase(it);
      } else {
        ++it;
      }
    }

    xSemaphoreGive(m_mutex);

    vTaskDelay(pdMS_TO_TICKS(1));
  }

  m_workerHandle = nullptr;
  vTaskDelete(nullptr);
}

bool GPIO::StartWorker(BaseType_t coreId, UBaseType_t priority) {
  if (m_workerHandle != nullptr) {
    return true;
  }

  if (m_mutex == nullptr) {
    m_mutex = xSemaphoreCreateMutex();
  }

  m_isWorkerRunning = true;

  return xTaskCreatePinnedToCore(
    WorkerTask,
    "GpioWorker",
    4096,
    nullptr,
    priority,
    &m_workerHandle,
    coreId
  ) == pdPASS;
}

void GPIO::StopWorker() {
  m_isWorkerRunning = false;
}

bool GPIO::ClearQueueEntries() {
  if (m_mutex == nullptr) {
    return false;
  }

  xSemaphoreTake(m_mutex, portMAX_DELAY);

  m_jobs.erase(
    std::remove_if(
      m_jobs.begin(),
      m_jobs.end(),
      [this](const GpioJob& job) {
        return job.owner == this;
      }
    ),
    m_jobs.end()
  );

  xSemaphoreGive(m_mutex);

  return true;
}

bool GPIO::ClearAllQueueEntries() {
  if (m_mutex == nullptr) {
    return false;
  }

  xSemaphoreTake(m_mutex, portMAX_DELAY);
  m_jobs.clear();
  xSemaphoreGive(m_mutex);

  return true;
}