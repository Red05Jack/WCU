#include "State.h"


State::State(const std::string& mainDir, const std::string& fileName) : m_mainDir(mainDir), m_fileName(fileName) {
  // TODO
}


State::STATE State::GetState() {
  // TODO
  return CLOSE;
}


bool State::SetState(STATE state) {
  // TODO
  return false;
}


bool State::ReopenFile() {
  // TODO
  return false;
}
