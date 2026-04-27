idf_component_register(
  SRCS "WCU.cpp"
  INCLUDE_DIRS "."
  REQUIRES core
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20)
