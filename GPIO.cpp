#include "GPIO.h"

#include "pico/stdlib.h"

GPIO* GPIO::m_instance = nullptr;
std::vector<Pin> GPIO::m_queue;
SemaphoreHandle_t GPIO::m_mutex = nullptr;


void GPIO::MakeInstance() {
  if (m_instance == nullptr) {
    m_instance = new GPIO();
  }
}


GPIO& GPIO::GetInstance() {
  MakeInstance();
  return *m_instance;
}


void GPIO::AddPinToQueue(const Pin& pin) {
  xSemaphoreTake(m_mutex, portMAX_DELAY);
  m_queue.push_back(pin);
  xSemaphoreGive(m_mutex);
}


void GPIO::Worker(void* arg) {
  while (true) {
    xSemaphoreTake(m_mutex, portMAX_DELAY);

    if (!m_queue.empty()) {
      int64_t currentTime = esp_timer_get_time();

      auto it = m_queue.begin();

      while (it != m_queue.end()) {
        bool functionInterrupt = (it->m_function == nullptr) || it->m_function();
        bool timeInterrupt = (it->m_delay != 0) && (currentTime >= it->m_delay);

        if (!functionInterrupt || timeInterrupt) {
          gpio_set_level((gpio_num_t)it->m_pin, it->m_state);
          it = m_queue.erase(it);
        } else {
          ++it;
        }
      }
    }

    xSemaphoreGive(m_mutex);
    vTaskDelay(pdMS_TO_TICKS(1)); // wichtig! sonst 100% CPU
  }
}


GPIO::GPIO() {
  m_mutex = xSemaphoreCreateMutex();
}


GPIO::~GPIO() {
  if (m_mutex) {
    vSemaphoreDelete(m_mutex);
  }

  if (m_instance != nullptr) {
    delete m_instance;
    m_instance = nullptr;
  }
}
