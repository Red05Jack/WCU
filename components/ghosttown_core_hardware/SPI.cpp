#include "SPI.h"

SPI::SPI() {
  m_mutex = xSemaphoreCreateMutex();
}

SPI::~SPI() {
  if (m_deviceHandle != nullptr) {
    spi_bus_remove_device(m_deviceHandle);
    m_deviceHandle = nullptr;
  }

  if (m_isBusInitialized) {
    spi_bus_free(m_host);
    m_isBusInitialized = false;
  }

  if (m_mutex != nullptr) {
    vSemaphoreDelete(m_mutex);
    m_mutex = nullptr;
  }
}

bool SPI::InitBus(spi_host_device_t host, gpio_num_t mosiPin, gpio_num_t misoPin, gpio_num_t sclkPin) {
  m_host = host;

  spi_bus_config_t busConfig = {};
  busConfig.mosi_io_num = mosiPin;
  busConfig.miso_io_num = misoPin;
  busConfig.sclk_io_num = sclkPin;
  busConfig.quadwp_io_num = GPIO_NUM_NC;
  busConfig.quadhd_io_num = GPIO_NUM_NC;
  busConfig.max_transfer_sz = 4096;

  const esp_err_t result = spi_bus_initialize(m_host, &busConfig, SPI_DMA_CH_AUTO);

  m_isBusInitialized = result == ESP_OK;

  return m_isBusInitialized;
}

bool SPI::AddDevice(gpio_num_t csPin, int clockSpeedHz, uint8_t mode) {
  if (!m_isBusInitialized) {
    return false;
  }

  spi_device_interface_config_t deviceConfig = {};
  deviceConfig.clock_speed_hz = clockSpeedHz;
  deviceConfig.mode = mode;
  deviceConfig.spics_io_num = csPin;
  deviceConfig.queue_size = 8;

  return spi_bus_add_device(m_host, &deviceConfig, &m_deviceHandle) == ESP_OK;
}

bool SPI::Transfer(const uint8_t* txData, uint8_t* rxData, size_t length) {
  if (m_deviceHandle == nullptr || length == 0) {
    return false;
  }

  spi_transaction_t transaction = {};
  transaction.length = length * 8;
  transaction.tx_buffer = txData;
  transaction.rx_buffer = rxData;

  xSemaphoreTake(m_mutex, portMAX_DELAY);
  const esp_err_t result = spi_device_transmit(m_deviceHandle, &transaction);
  xSemaphoreGive(m_mutex);

  return result == ESP_OK;
}