idf_component_register(
  SRCS
    "State.cpp"

  INCLUDE_DIRS
    "."

  REQUIRES
    nvs_flash
    log
)