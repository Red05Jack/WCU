#include "State.h"

#include <cstring>


State::State(const std::string& path) {
  m_state = STATE::CLOSE;
}


State::STATE State::GetState() {
  return m_state;
}


bool State::SetState(STATE state) {
  m_state = state;
  return true;
}
