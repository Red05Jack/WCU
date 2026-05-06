#include "State.h"

#include <cstring>

#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"

static const char* TAG = "State";

static constexpr const char* NamespaceName = "state";
static constexpr const char* BlobKey = "states";

State* State::m_instance = nullptr;
uint8_t State::m_states[STATE_STORAGE_SIZE] = {};

void State::MakeInstance() {
  if (m_instance == nullptr) {
    m_instance = new State();
  }
}

State& State::GetInstance() {
  MakeInstance();
  return *m_instance;
}

State::State() {
  esp_err_t result = nvs_flash_init();

  if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    result = nvs_flash_init();
  }

  if (result != ESP_OK) {
    ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(result));
    return;
  }

  result = nvs_open(NamespaceName, NVS_READWRITE, &m_nvsHandle);

  if (result != ESP_OK) {
    ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(result));
    return;
  }

  m_isInitialized = true;

  if (!LoadStates()) {
    ResetStates();
    SaveStates();
  }
}

State::~State() {
  if (m_isInitialized) {
    nvs_close(m_nvsHandle);
    m_isInitialized = false;
  }
}

bool State::SetState(uint8_t value, uint8_t position) {
  if (position >= STATE_STORAGE_SIZE - STATE_CONTROL_BYTES_SIZE) {
    return false;
  }

  m_states[position] = value;
  return SaveStates();
}

uint8_t State::GetState(uint8_t position) const {
  if (position >= STATE_STORAGE_SIZE - STATE_CONTROL_BYTES_SIZE) {
    return 0;
  }

  return m_states[position];
}

bool State::SetStates(const uint8_t* data, size_t length) {
  if (data == nullptr || length > STATE_STORAGE_SIZE - STATE_CONTROL_BYTES_SIZE) {
    return false;
  }

  memcpy(m_states, data, length);
  return SaveStates();
}

bool State::GetStates(uint8_t* data, size_t length) const {
  if (data == nullptr || length > STATE_STORAGE_SIZE - STATE_CONTROL_BYTES_SIZE) {
    return false;
  }

  memcpy(data, m_states, length);
  return true;
}

bool State::SaveStates() {
  if (!m_isInitialized) {
    return false;
  }

  for (size_t i = 0; i < STATE_CONTROL_BYTES_SIZE; i++) {
    m_states[STATE_STORAGE_SIZE - STATE_CONTROL_BYTES_SIZE + i] = m_controlBytes[i];
  }

  esp_err_t result = nvs_set_blob(
    m_nvsHandle,
    BlobKey,
    m_states,
    STATE_STORAGE_SIZE
  );

  if (result != ESP_OK) {
    ESP_LOGE(TAG, "NVS set blob failed: %s", esp_err_to_name(result));
    return false;
  }

  result = nvs_commit(m_nvsHandle);

  if (result != ESP_OK) {
    ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(result));
    return false;
  }

  return true;
}

bool State::LoadStates() {
  if (!m_isInitialized) {
    return false;
  }

  size_t requiredSize = STATE_STORAGE_SIZE;

  esp_err_t result = nvs_get_blob(
    m_nvsHandle,
    BlobKey,
    m_states,
    &requiredSize
  );

  if (result != ESP_OK || requiredSize != STATE_STORAGE_SIZE) {
    return false;
  }

  for (size_t i = 0; i < STATE_CONTROL_BYTES_SIZE; i++) {
    if (m_states[STATE_STORAGE_SIZE - STATE_CONTROL_BYTES_SIZE + i] != m_controlBytes[i]) {
      return false;
    }
  }

  return true;
}

void State::ResetStates() {
  memset(m_states, 0, STATE_STORAGE_SIZE);

  for (size_t i = 0; i < STATE_CONTROL_BYTES_SIZE; i++) {
    m_states[STATE_STORAGE_SIZE - STATE_CONTROL_BYTES_SIZE + i] = m_controlBytes[i];
  }
}