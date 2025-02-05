#pragma once

#include "State.h"


class Window {
public:
  Window();
  ~Window() = default;


  // Public Member Methods
  bool Toogle();
  bool Open();
  bool Close();

  void SetState(State::STATE state);
  State::STATE GetState();


private:
  // Private Member Variables
  State m_state;


  // Private Member Methods
  bool OpenHardware();
  bool CloseHardware();


};
