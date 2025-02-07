#include <iostream>

#include "Window.h"
#include "State.h"


#define WINDOW_STATE_DIR "./WindowStates"


int main() {
  Window leftWindow = Window(WINDOW_STATE_DIR, "StateLeftWindow");
  Window rightWindow = Window(WINDOW_STATE_DIR, "StateRightWindow");




  return 0;
}
