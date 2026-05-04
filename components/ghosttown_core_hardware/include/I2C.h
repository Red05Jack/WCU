// I2C.h
#pragma once

#include <cstdint>
#include <vector>

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class I2C {
public:
  I2C();
  ~I2C();

  bool Init(i2c_port_num_t port, gpio_num_t sdaPin, gpio_num_t sclPin, uint32_t frequencyHz = 400000);

  bool Write(uint8_t address, const uint8_t* data, size_t length);
  bool Read(uint8_t address, uint8_t* data, size_t length);
  bool WriteRead(uint8_t address, const uint8_t* txData, size_t txLength, uint8_t* rxData, size_t rxLength);

private:
  bool AddDevice(uint8_t address, i2c_master_dev_handle_t* deviceHandle);

private:
  i2c_master_bus_handle_t m_busHandle = nullptr;
  SemaphoreHandle_t m_mutex = nullptr;
  uint32_t m_frequencyHz = 400000;
};