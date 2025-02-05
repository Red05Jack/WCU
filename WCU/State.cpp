#include "State.h"


State::State(std::string path) : m_path(path) {

}


State::STATE State::GetState() {
  return CLOSE;
}


bool State::SetState(STATE state) {
  return false;
}


bool State::ReopenFile() {
  return false;
}
