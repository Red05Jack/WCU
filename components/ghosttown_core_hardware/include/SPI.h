// SPI.h
#pragma once

#include <cstdint>

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class SPI {
public:
  SPI();
  ~SPI();

  bool InitBus(spi_host_device_t host, gpio_num_t mosiPin, gpio_num_t misoPin, gpio_num_t sclkPin);
  bool AddDevice(gpio_num_t csPin, int clockSpeedHz, uint8_t mode = 0);

  bool Transfer(const uint8_t* txData, uint8_t* rxData, size_t length);

private:
  spi_host_device_t m_host = SPI2_HOST;
  spi_device_handle_t m_deviceHandle = nullptr;
  SemaphoreHandle_t m_mutex = nullptr;
  bool m_isBusInitialized = false;
};