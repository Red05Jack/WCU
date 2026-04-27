#pragma once

#include <vector>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"


// Pin      GPIO pin number that will be affected.
// Delay    Absolute timestamp in microseconds at which the state change is executed.
//          To execute the change immediately, pass the current machine timestamp.
//          If set to 0, timing is ignored and only the function condition is used.
// State    GPIO state that will be applied (true = high, false = low).
// Function Optional callback function.
//          If provided, the state change is executed as soon as the function returns false.
//          If nullptr, only the time condition is evaluated.
struct Pin {
  Pin(
    const uint8_t pin,
    const uint64_t delay,
    const bool state,
    bool (*function)()
  ) :
    m_pin(pin),
    m_delay(delay),
    m_state(state),
    m_function(function)
  {}

  uint8_t m_pin;
  uint64_t m_delay;
  bool m_state;
  bool (*m_function)();
};


class GPIO {
public:
  GPIO(const GPIO& obj) = delete;
    

  // Public Member Methods
  static void MakeInstance();
  static GPIO& GetInstance();

  void AddPinToQueue(const Pin& pin);
  static void Worker(void* arg); // FreeRTOS task


private:
  GPIO();
  ~GPIO();
  

  // Private Member Variables
  static GPIO* m_instance;
  static std::vector<Pin> m_queue;
  static SemaphoreHandle_t m_mutex;


};
