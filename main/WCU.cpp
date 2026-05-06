#include <memory>

#include "MCP23017.h"
#include "I2C.h"
#include "DigitalPin.h"
#include "State.h"
#include "Window.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char* TAG = "WCU";

static constexpr uint8_t MCP23017_A_ADDRESS = 0x20;
static constexpr uint8_t MCP23017_B_ADDRESS = 0x21;

// MCP23017 A

static constexpr uint8_t LF_PWL = 0; // A PA0

static constexpr uint8_t LF_LFOpen = 3;     // A PA3
static constexpr uint8_t LF_LFOpenAll = 4;  // A PA4
static constexpr uint8_t LF_LFCloseAll = 1; // A PA1
static constexpr uint8_t LF_LFClose = 2;    // A PA2

static constexpr uint8_t LF_RFOpen = 8;     // A PB0
static constexpr uint8_t LF_RFOpenAll = 9;  // A PB1
static constexpr uint8_t LF_RFCloseAll = 6; // A PA6
static constexpr uint8_t LF_RFClose = 7;    // A PA7

static constexpr uint8_t LF_LRToggle = 5;   // A PA5
static constexpr uint8_t LF_RRToggle = 10;  // A PB2

// MCP23017 B

static constexpr uint8_t RF_RFOpen = 10;     // B PB2
static constexpr uint8_t RF_RFOpenAll = 11;  // B PB3
static constexpr uint8_t RF_RFCloseAll = 8;  // B PB0
static constexpr uint8_t RF_RFClose = 9;     // B PB1

static constexpr uint8_t LR_LRToggle = 12;   // B PB4
static constexpr uint8_t RR_RRToggle = 13;   // B PB5

static constexpr uint8_t LF_Motor_A = 0; // B PA0
static constexpr uint8_t LF_Motor_B = 1; // B PA1

static constexpr uint8_t RF_Motor_A = 2; // B PA2
static constexpr uint8_t RF_Motor_B = 3; // B PA3

static constexpr uint8_t LR_Motor_A = 4; // B PA4
static constexpr uint8_t LR_Motor_B = 5; // B PA5

static constexpr uint8_t RR_Motor_A = 6; // B PA6
static constexpr uint8_t RR_Motor_B = 7; // B PA7

extern "C" void app_main() {
  ESP_LOGI(TAG, "Starting...");

  I2C i2c;

  if (!i2c.Init(I2C_NUM_0, GPIO_NUM_4, GPIO_NUM_5, 400000)) {
    ESP_LOGE(TAG, "I2C init failed");
    return;
  }

  vTaskDelay(pdMS_TO_TICKS(100)); // Give bus time to stabilize

  MCP23017 expanderA;

  if (!expanderA.Init(&i2c, MCP23017_A_ADDRESS)) {
    ESP_LOGE(TAG, "MCP23017 A init failed");
    return;
  }

  ESP_LOGI(TAG, "MCP23017 A initialized");

  MCP23017 expanderB;

  if (!expanderB.Init(&i2c, MCP23017_B_ADDRESS)) {
    ESP_LOGE(TAG, "MCP23017 B init failed");
    return;
  }

  ESP_LOGI(TAG, "MCP23017 B initialized");

  // Windows

  State& state = State::GetInstance();

  WindowConfig configLF = {
    .statePos = 0,
    .timeOpenHighPos = 1,
    .timeOpenLowPos = 2,
    .timeCloseHighPos = 3,
    .timeCloseLowPos = 4,
    .state = &state
  };

  WindowConfig configRF = {
    .statePos = 5,
    .timeOpenHighPos = 6,
    .timeOpenLowPos = 7,
    .timeCloseHighPos = 8,
    .timeCloseLowPos = 9,
    .state = &state
  };

  WindowConfig configLR = {
    .statePos = 10,
    .timeOpenHighPos = 11,
    .timeOpenLowPos = 12,
    .timeCloseHighPos = 13,
    .timeCloseLowPos = 14,
    .state = &state
  };

  WindowConfig configRR = {
    .statePos = 15,
    .timeOpenHighPos = 16,
    .timeOpenLowPos = 17,
    .timeCloseHighPos = 18,
    .timeCloseLowPos = 19,
    .state = &state
  };

  Window windowLF(configLF, expanderB.GetPinObject(LF_Motor_A), expanderB.GetPinObject(LF_Motor_B));
  Window windowRF(configRF, expanderB.GetPinObject(RF_Motor_A), expanderB.GetPinObject(RF_Motor_B));
  Window windowLR(configLR, expanderB.GetPinObject(LR_Motor_A), expanderB.GetPinObject(LR_Motor_B));
  Window windowRR(configRR, expanderB.GetPinObject(RR_Motor_A), expanderB.GetPinObject(RR_Motor_B));

  windowLF.AddInput(WindowAction::Open, expanderA.GetPinObject(LF_LFOpen));
  windowLF.AddInput(WindowAction::OpenAll, expanderA.GetPinObject(LF_LFOpenAll));
  windowLF.AddInput(WindowAction::CloseAll, expanderA.GetPinObject(LF_LFCloseAll));
  windowLF.AddInput(WindowAction::Close, expanderA.GetPinObject(LF_LFClose));

  windowRF.AddInput(WindowAction::Open, expanderA.GetPinObject(LF_RFOpen));
  windowRF.AddInput(WindowAction::OpenAll, expanderA.GetPinObject(LF_RFOpenAll));
  windowRF.AddInput(WindowAction::CloseAll, expanderA.GetPinObject(LF_RFCloseAll));
  windowRF.AddInput(WindowAction::Close, expanderA.GetPinObject(LF_RFClose));
  windowRF.AddInput(WindowAction::Open, expanderB.GetPinObject(RF_RFOpen), expanderA.GetPinObject(LF_PWL), false, false);
  windowRF.AddInput(WindowAction::OpenAll, expanderB.GetPinObject(RF_RFOpenAll), expanderA.GetPinObject(LF_PWL), false, false);
  windowRF.AddInput(WindowAction::CloseAll, expanderB.GetPinObject(RF_RFCloseAll), expanderA.GetPinObject(LF_PWL), false, false);
  windowRF.AddInput(WindowAction::Close, expanderB.GetPinObject(RF_RFClose), expanderA.GetPinObject(LF_PWL), false, false);

  windowLR.AddInput(WindowAction::Toggle, expanderA.GetPinObject(LF_LRToggle));
  windowLR.AddInput(WindowAction::Toggle, expanderB.GetPinObject(LR_LRToggle), expanderA.GetPinObject(LF_PWL), false, false);

  windowRR.AddInput(WindowAction::Toggle, expanderA.GetPinObject(LF_RRToggle));
  windowRR.AddInput(WindowAction::Toggle, expanderB.GetPinObject(RR_RRToggle), expanderA.GetPinObject(LF_PWL), false, false);

  windowLF.CalibrateWindows();
  windowRF.CalibrateWindows();
  windowLR.CalibrateWindows();
  windowRR.CalibrateWindows();

  if (!expanderA.StartInterruptsWithPolling(20)) {
    ESP_LOGE(TAG, "MCP23017 A polling worker failed");
    return;
  }

  if (!expanderB.StartInterruptsWithPolling(20)) {
    ESP_LOGE(TAG, "MCP23017 B polling worker failed");
    return;
  }

  ESP_LOGI(TAG, "MCP23017 A and B polling worker started");

  windowLF.StartWindow();
  windowRF.StartWindow();
  windowLR.StartWindow();
  windowRR.StartWindow();

  ESP_LOGI(TAG, "System running");

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
