#include "MCP2515.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char *TAG = "CAN_S3";

static constexpr uint32_t CAN_ID_S3_TO_C3 = 0x18FF50E5;
static constexpr uint32_t CAN_ID_C3_TO_S3 = 0x18FF51C3;

static constexpr TickType_t SEND_INTERVAL_TICKS = pdMS_TO_TICKS(5000);

static constexpr Mcp2515Config canConfig = {
    .pinMosi = GPIO_NUM_11,
    .pinMiso = GPIO_NUM_13,
    .pinSck = GPIO_NUM_12,
    .pinCs = GPIO_NUM_10,
    .pinInt = GPIO_NUM_9,

    .spiHost = SPI2_HOST,
    .spiClockHz = 1 * 1000 * 1000,

    .canBitrate = 500000,
    .oscillatorHz = 8000000,

    .acceptAllFrames = true,
    .enableRxInterrupts = true
};

extern "C" void app_main() {
    Mcp2515 can(canConfig, TAG);

    if (!can.Init()) {
        ESP_LOGE(TAG, "MCP2515 init failed");
        return;
    }

    ESP_LOGI(TAG, "S3 CAN ready");

    uint32_t counter = 0;
    TickType_t lastSend = xTaskGetTickCount();

    while (true) {
        CanFrame frame;

        while (can.ReadFrame(frame)) {
            ESP_LOGI(
                TAG,
                "RX id=0x%lX ext=%d dlc=%u data=%02X %02X %02X %02X %02X %02X %02X %02X",
                frame.id,
                frame.isExtended,
                frame.dlc,
                frame.data[0],
                frame.data[1],
                frame.data[2],
                frame.data[3],
                frame.data[4],
                frame.data[5],
                frame.data[6],
                frame.data[7]
            );

            if (frame.isExtended && frame.id == CAN_ID_C3_TO_S3) {
                ESP_LOGI(TAG, "Received answer from C3, counter=%lu", counter);
            }
        }

        TickType_t now = xTaskGetTickCount();

        if (now - lastSend >= SEND_INTERVAL_TICKS) {
            lastSend += SEND_INTERVAL_TICKS;

            uint8_t data[8] = {
                0x53, // 'S'
                0x33, // '3'
                static_cast<uint8_t>(counter >> 24),
                static_cast<uint8_t>(counter >> 16),
                static_cast<uint8_t>(counter >> 8),
                static_cast<uint8_t>(counter),
                0xAA,
                0x55
            };

            bool hasSent = can.SendExtended(CAN_ID_S3_TO_C3, data, 8);

            if (hasSent) {
                ESP_LOGI(TAG, "TX request to C3 id=0x%lX counter=%lu", CAN_ID_S3_TO_C3, counter);
                counter++;
            } else {
                ESP_LOGW(TAG, "TX request failed");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}