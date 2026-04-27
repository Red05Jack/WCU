#include "I2C.h"


I2CMaster::I2CMaster(i2c_port_t port) : m_port(port) {
    m_mutex = xSemaphoreCreateMutex();
}


esp_err_t I2CMaster::Write(uint8_t addr, uint8_t reg, uint8_t data) {
    xSemaphoreTake(m_mutex, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(m_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(m_mutex);
    return ret;
}


esp_err_t I2CMaster::Read(uint8_t addr, uint8_t reg, uint8_t& data) {
    xSemaphoreTake(m_mutex, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(m_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(m_mutex);
    return ret;
}
