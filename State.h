#pragma once

#include <cstdint>
#include <string>


class State {
public:
  State(const std::string& path);
  ~State() = default;


  // Public Member Enums
  enum STATE {
    OPEN = 1,
    CLOSE = 0
  };


  // Public Member Methods
  STATE GetState();
  bool SetState(STATE state);


private:
  // Private Member Variables
  STATE m_state;


};
