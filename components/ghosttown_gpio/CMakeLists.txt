idf_component_register(
    SRCS
      "GPIO.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
      esp_driver_gpio
      esp_timer
)
