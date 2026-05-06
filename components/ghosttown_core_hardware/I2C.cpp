#include "I2C.h"

I2C::I2C() {
  m_mutex = xSemaphoreCreateMutex();
}

I2C::~I2C() {
  if (m_busHandle != nullptr) {
    i2c_del_master_bus(m_busHandle);
    m_busHandle = nullptr;
  }

  if (m_mutex != nullptr) {
    vSemaphoreDelete(m_mutex);
    m_mutex = nullptr;
  }
}

bool I2C::Init(i2c_port_num_t port, gpio_num_t sdaPin, gpio_num_t sclPin, uint32_t frequencyHz) {
  m_frequencyHz = frequencyHz;

  i2c_master_bus_config_t busConfig = {};
  busConfig.i2c_port = port;
  busConfig.sda_io_num = sdaPin;
  busConfig.scl_io_num = sclPin;
  busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
  busConfig.glitch_ignore_cnt = 7;
  busConfig.flags.enable_internal_pullup = true;

  return i2c_new_master_bus(&busConfig, &m_busHandle) == ESP_OK;
}

bool I2C::AddDevice(uint8_t address, i2c_master_dev_handle_t* deviceHandle) {
  if (m_busHandle == nullptr || deviceHandle == nullptr) {
    return false;
  }

  i2c_device_config_t deviceConfig = {};
  deviceConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  deviceConfig.device_address = address;
  deviceConfig.scl_speed_hz = m_frequencyHz;

  return i2c_master_bus_add_device(m_busHandle, &deviceConfig, deviceHandle) == ESP_OK;
}

bool I2C::Write(uint8_t address, const uint8_t* data, size_t length) {
  if (m_busHandle == nullptr || data == nullptr || length == 0) {
    return false;
  }

  xSemaphoreTake(m_mutex, portMAX_DELAY);

  i2c_master_dev_handle_t deviceHandle = nullptr;

  if (!AddDevice(address, &deviceHandle)) {
    xSemaphoreGive(m_mutex);
    return false;
  }

  const esp_err_t result = i2c_master_transmit(deviceHandle, data, length, pdMS_TO_TICKS(1000));

  i2c_master_bus_rm_device(deviceHandle);

  xSemaphoreGive(m_mutex);

  return result == ESP_OK;
}

bool I2C::Read(uint8_t address, uint8_t* data, size_t length) {
  if (m_busHandle == nullptr || data == nullptr || length == 0) {
    return false;
  }

  xSemaphoreTake(m_mutex, portMAX_DELAY);

  i2c_master_dev_handle_t deviceHandle = nullptr;

  if (!AddDevice(address, &deviceHandle)) {
    xSemaphoreGive(m_mutex);
    return false;
  }

  const esp_err_t result = i2c_master_receive(deviceHandle, data, length, pdMS_TO_TICKS(1000));

  i2c_master_bus_rm_device(deviceHandle);

  xSemaphoreGive(m_mutex);

  return result == ESP_OK;
}

bool I2C::WriteRead(
  uint8_t address,
  const uint8_t* txData,
  size_t txLength,
  uint8_t* rxData,
  size_t rxLength
) {
  if (
    m_busHandle == nullptr ||
    txData == nullptr ||
    txLength == 0 ||
    rxData == nullptr ||
    rxLength == 0
  ) {
    return false;
  }

  xSemaphoreTake(m_mutex, portMAX_DELAY);

  i2c_master_dev_handle_t deviceHandle = nullptr;

  if (!AddDevice(address, &deviceHandle)) {
    xSemaphoreGive(m_mutex);
    return false;
  }

  const esp_err_t result = i2c_master_transmit_receive(
    deviceHandle,
    txData,
    txLength,
    rxData,
    rxLength,
    pdMS_TO_TICKS(1000)
  );

  i2c_master_bus_rm_device(deviceHandle);

  xSemaphoreGive(m_mutex);

  return result == ESP_OK;
}