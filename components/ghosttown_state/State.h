#pragma once

#include <cstdint>
#include <cstddef>

#include "nvs.h"

#define STATE_STORAGE_SIZE 256
#define STATE_CONTROL_BYTES_SIZE 8

class State {
public:
  State(const State& obj) = delete;
  State& operator=(const State& obj) = delete;

  static void MakeInstance();
  static State& GetInstance();

  bool SetState(uint8_t value, uint8_t position);
  uint8_t GetState(uint8_t position) const;

  bool SetStates(const uint8_t* data, size_t length);
  bool GetStates(uint8_t* data, size_t length) const;

private:
  State();
  ~State();

  bool SaveStates();
  bool LoadStates();
  void ResetStates();

private:
  static State* m_instance;
  static uint8_t m_states[STATE_STORAGE_SIZE];

  nvs_handle_t m_nvsHandle = 0;
  bool m_isInitialized = false;

  uint8_t m_controlBytes[STATE_CONTROL_BYTES_SIZE] = {1, 0, 0, 1, 1, 0, 0, 1};
};