idf_component_register(
    SRCS
      "I2C.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
      esp_driver_i2c
      freertos
      esp_timer
)
