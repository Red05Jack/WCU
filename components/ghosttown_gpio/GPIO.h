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

constexpr bool in 0;
constexpr bool out 1;

constexpr int8_t pullDown -1;
constexpr int8_t noPullResistor 0;
constexpr int8_t pullUp 1;

constexpr bool noInterrupts 0;
constexpr bool interrupts 1;


class GPIO {
public:
  GPIO();
  GPIO(uint8_t pin, bool mode, int8_t pullResistor, bool interrupts);
  ~GPIO();


  // Public Member Methods
  bool SetPin(uint8_t pin);
  bool SetMode(bool mode);
  bool SetPullResistor(int8_t pullResistor);
  bool SetInterrupts(bool interrupts);

  bool Set(); // High Low
  bool Set(); // High Low + Time
  bool Set(); // High Low + Func
  bool Set(); // High Low + Func + Time

  int16_t Get();
  bool Get(); // Func + State

  bool ClearQueueEntries(); // Alle von dem Object
  bool ClearAllQueueEntries(); // Alle Alle


protected:
  // Static Protected Member Variables
  static std::vector<> m_queue;


  // Static Protected Member Methods
  static void Worker();
  
  
  // Protected Member Variables
  gpio_config_t m_config;

  
  // Protected Member Methods
  bool SaveConfig();


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
