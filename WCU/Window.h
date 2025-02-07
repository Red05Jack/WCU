#pragma once

#include "State.h"


class Window {
public:
  Window(const std::string& mainDir, const std::string& fileName);
  ~Window() = default;


  // Public Member Methods
  bool Toogle();
  bool Open();
  bool Close();

  bool ToogleState();
  bool OpenState();
  bool CloseState();


private:
  // Private Member Variables
  State m_state;


  // Private Member Methods
  bool OpenHardware();
  bool CloseHardware();


};
