idf_component_register(
  SRCS
    "MCP2515.cpp"

  INCLUDE_DIRS
    "."

  REQUIRES
    ghosttown_core_hardware
    esp_driver_gpio
    esp_driver_spi
    esp_timer
    freertos
    log
)