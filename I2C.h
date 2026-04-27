#pragma once

#include "driver/i2c.h"
#include "freertos/semphr.h"


class I2CMaster {
public:
    I2CMaster(i2c_port_t port);


    esp_err_t Write(uint8_t addr, uint8_t reg, uint8_t data);
    esp_err_t Read(uint8_t addr, uint8_t reg, uint8_t& data);


private:
    i2c_port_t m_port;
    SemaphoreHandle_t m_mutex;


};
