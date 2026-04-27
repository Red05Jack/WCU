#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdint.h>


class I2CMaster {
public:
    I2CMaster(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t clk_hz = 400000);
    ~I2CMaster();


    esp_err_t Write(uint8_t addr, uint8_t reg, uint8_t data);
    esp_err_t Read(uint8_t addr, uint8_t reg, uint8_t &data);


private:
    i2c_master_bus_handle_t m_bus;
    SemaphoreHandle_t m_mutex;


};
