idf_component_register(
  SRCS
    "WCU.cpp"
  INCLUDE_DIRS "."
  REQUIRES
    ghosttown_gpio
    ghosttown_i2c
    ghosttown_spi
)
