idf_component_register(
  SRCS
    "MCP23017.cpp"

  INCLUDE_DIRS
    "."

  REQUIRES
    ghosttown_core_hardware
    freertos
    log
)