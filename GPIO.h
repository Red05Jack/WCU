#pragma once

#include <vector>
#include "pico/sync.h"


struct Pin {
  Pin(const uint8_t pin, const uint64_t delay, const bool state) : m_pin(pin), m_delay(delay), m_state(state) {}

  uint8_t m_pin;
  uint64_t m_delay;
  bool m_state;
};


class GPIO {
public:
  GPIO(const GPIO& obj) = delete;
    

  // Public Member Methods
  static void MakeInstance();
  static GPIO& GetInstance();

  void AddPinToQueue(const Pin& pin);
  static void Worker();


private:
  GPIO();
  ~GPIO();
  

  // Private Member Variables
  static GPIO* m_instance;
  static std::vector<Pin> m_queue;
  static critical_section_t m_criticalSection;


};
