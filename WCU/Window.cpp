#include "Window.h"


Window::Window(
  const std::string& mainDir,
  const std::string& fileName
) : m_state(State(mainDir, fileName)) {}


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


bool Window::ToogleState() {
  if (m_state.GetState() == State::OPEN) {
    return m_state.SetState(State::CLOSE);
  }

  if (m_state.GetState() == State::CLOSE) {
    return m_state.SetState(State::OPEN);
  }

  return false;
}


bool Window::OpenState() {
  return m_state.SetState(State::OPEN);
}


bool Window::CloseState() {
  return m_state.SetState(State::CLOSE);
}


bool Window::OpenHardware() {
  // TODO
  return false;
}


bool Window::CloseHardware() {
  // TODO
  return false;
}
