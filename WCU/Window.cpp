#include "Window.h"


Window::Window() {

}


bool Window::Toogle() {
  if (m_state.GetState() == State::OPEN) {
    if (CloseHardware()) {
      m_state.SetState(State::CLOSE);
      return true;
    }
  }

  if (m_state.GetState() == State::CLOSE) {
    if (OpenHardware()) {
      m_state.SetState(State::OPEN);
      return true;
    }
  }
  
  return false;
}


bool Window::Open() {
  if (m_state.GetState() == State::OPEN) {
    if (OpenHardware()) {
      m_state.SetState(State::OPEN);
      return true;
    }
  }

  return false;
}


bool Window::Close() {
  if (m_state.GetState() == State::OPEN) {
    if (CloseHardware()) {
      m_state.SetState(State::CLOSE);
      return true;
    }
  }
  
  return false;
}


void Window::SetState(State::STATE state) {
  m_state.SetState(state);
}


State::STATE Window::GetState() {
  return m_state.GetState();
}


bool Window::OpenHardware() {
  return false;
}


bool Window::CloseHardware() {
  return false;
}
