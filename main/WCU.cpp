#include <memory>

#include "MCP23017.h"
#include "I2C.h"
#include "DigitalPin.h"
#include "State.h"
#include "Window.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char* TAG = "WINDOW_TEST";

static constexpr uint8_t MCP23017_ADDRESS = 0x27;

static constexpr uint8_t ButtonPa0 = 0;
static constexpr uint8_t ButtonPa1 = 1;
static constexpr uint8_t ButtonPa2 = 2;
static constexpr uint8_t ButtonPa3 = 3;

static constexpr uint8_t LedPb0 = 8;
static constexpr uint8_t LedPb1 = 9;

extern "C" void app_main() {
  ESP_LOGI(TAG, "Starting...");

  I2C i2c;

  if (!i2c.Init(I2C_NUM_0, GPIO_NUM_4, GPIO_NUM_5, 400000)) {
    ESP_LOGE(TAG, "I2C init failed");
    return;
  }

  vTaskDelay(pdMS_TO_TICKS(100)); // Give bus time to stabilize

  MCP23017 expander;

  if (!expander.Init(&i2c, MCP23017_ADDRESS)) {
    ESP_LOGE(TAG, "MCP23017 init failed");
    return;
  }

  ESP_LOGI(TAG, "MCP23017 initialized");

  // --- Pins holen ---
  auto buttonOpen     = expander.GetPinObject(ButtonPa0);
  auto buttonOpenAll  = expander.GetPinObject(ButtonPa1);
  auto buttonCloseAll = expander.GetPinObject(ButtonPa2);
  auto buttonClose    = expander.GetPinObject(ButtonPa3);

  auto motorClose = expander.GetPinObject(LedPb0);
  auto motorOpen  = expander.GetPinObject(LedPb1);

  if (
    !buttonOpen || !buttonOpenAll ||
    !buttonClose || !buttonCloseAll ||
    !motorOpen || !motorClose
  ) {
    ESP_LOGE(TAG, "Pin object creation failed");
    return;
  }

  // --- Window Setup ---
  State& state = State::GetInstance();

  WindowConfig config = {
    .statePos = 0,
    .timeOpenHighPos = 1,
    .timeOpenLowPos = 2,
    .timeCloseHighPos = 3,
    .timeCloseLowPos = 4,
    .state = &state
  };

  Window window(
    config,
    buttonOpen,
    buttonOpenAll,
    buttonClose,
    buttonCloseAll,
    motorOpen,
    motorClose
  );

  // --- Default Zeiten setzen ---
  if (!window.CalibrateWindows()) {
    ESP_LOGE(TAG, "Calibration failed");
    return;
  }

  // --- WICHTIG: Interrupt Worker ERST danach starten ---
  if (!expander.StartInterruptsWithPolling(20)) {
    ESP_LOGE(TAG, "Polling interrupt start failed");
    return;
  }

  ESP_LOGI(TAG, "MCP23017 polling worker started");

  // --- Window starten (nutzt DigitalPin interrupts) ---
  if (!window.StartWindow()) {
    ESP_LOGE(TAG, "Window start failed");
    return;
  }

  ESP_LOGI(TAG, "System running");

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}