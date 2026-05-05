#pragma once

#include <memory>

#include "DigitalPin.h"
#include "State.h"



struct WindowConfig {
  uint8_t statePos;

  uint8_t timeOpenHighPos;
  uint8_t timeOpenLowPos;
  uint8_t timeCloseHighPos;
  uint8_t timeCloseLowPos;
 
  State& state;
};


class Window {
public:
  Window();
  Window() = default;



  void StartWindow(); // Init
  void StopWindow();



  void StartOpen();
  void StartClose();

  void Stop();

  void OpenAll();
  void CloseAll();

  void Toogle();


  uint32_t GetTimeOpen(); //ms
  uint32_t GetTimeClose(); //ms

  uint8_t GetCurrentState(); //0-255
  bool SetCurrentState(uint8_t state);

  bool CalibrateWindows();


private:

  bool SetTimeOpen(uint32_t time); //ms
  bool SetTimeClose(uint32_t time);





  std::shared_ptr<DigitalPin> m_buttonOpen;
  std::shared_ptr<DigitalPin> m_buttonOpenAll;
  std::shared_ptr<DigitalPin> m_buttonClose;
  std::shared_ptr<DigitalPin> m_buttonCloseAll;

  std::shared_ptr<DigitalPin> m_motorA;
  std::shared_ptr<DigitalPin> m_motorB;




  
  WindowConfig config;



}
