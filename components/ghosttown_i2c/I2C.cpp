#include "I2C.h"

I2CMaster::I2CMaster(i2c_port_t port, gpio_num_t sda, gpio_num_t scl, uint32_t clk_hz) {
    m_mutex = xSemaphoreCreateMutex();

    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = port;
    bus_config.sda_io_num = sda;
    bus_config.scl_io_num = scl;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &m_bus));
}

I2CMaster::~I2CMaster() {
    if (m_bus) {
        i2c_del_master_bus(m_bus);
    }
    vSemaphoreDelete(m_mutex);
}


esp_err_t I2CMaster::Write(uint8_t addr, uint8_t reg, uint8_t data) {
    xSemaphoreTake(m_mutex, portMAX_DELAY);

    uint8_t buffer[2] = {reg, data};

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = addr;
    dev_cfg.scl_speed_hz = 400000;

    i2c_master_dev_handle_t dev;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(m_bus, &dev_cfg, &dev));

    esp_err_t ret = i2c_master_transmit(dev, buffer, sizeof(buffer), 1000 / portTICK_PERIOD_MS);

    i2c_master_bus_rm_device(dev);

    xSemaphoreGive(m_mutex);
    return ret;
}


esp_err_t I2CMaster::Read(uint8_t addr, uint8_t reg, uint8_t &data) {
    xSemaphoreTake(m_mutex, portMAX_DELAY);

    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = addr;
    dev_cfg.scl_speed_hz = 400000;

    i2c_master_dev_handle_t dev;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(m_bus, &dev_cfg, &dev));

    esp_err_t ret = i2c_master_transmit_receive(
        dev,
        &reg, 1,
        &data, 1,
        1000 / portTICK_PERIOD_MS
    );

    i2c_master_bus_rm_device(dev);

    xSemaphoreGive(m_mutex);
    return ret;
}