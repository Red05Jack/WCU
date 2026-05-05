idf_component_register(
  SRCS
    "WCU.cpp"
    "Window.cpp"
  INCLUDE_DIRS "."
  REQUIRES
    ghosttown_mcp2515
    ghosttown_mcp23017
    ghosttown_state
)
