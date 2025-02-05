#pragma once

#include <string>
#include <fstream>


class State {
public:
  State(std::string path);
  ~State() = default;


  // Public Member Enums
  enum STATE {
    OPEN,
    CLOSE
  };


  // Public Member Methods
  STATE GetState();
  bool SetState(STATE state);


private:
  // Private Member Variables
  std::string m_path;
  std::fstream m_file;


  // Private Member Methods
  bool ReopenFile();


};
