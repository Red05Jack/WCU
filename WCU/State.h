#pragma once

#include <string>
#include <fstream>


class State {
public:
  State(const std::string& mainDir, const std::string& fileName);
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
  std::string m_mainDir;
  std::string m_fileName;
  std::fstream m_file;


  // Private Member Methods
  bool ReopenFile();


};
