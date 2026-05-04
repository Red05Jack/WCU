#include "MCP2515.h"

#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

Mcp2515::Mcp2515(const Mcp2515Config &config, const char *tag)
    : m_config(config), m_tag(tag) {
}

bool Mcp2515::Init() {
    spi_bus_config_t busConfig = {};
    busConfig.mosi_io_num = m_config.pinMosi;
    busConfig.miso_io_num = m_config.pinMiso;
    busConfig.sclk_io_num = m_config.pinSck;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;

    spi_device_interface_config_t deviceConfig = {};
    deviceConfig.clock_speed_hz = m_config.spiClockHz;
    deviceConfig.mode = 0;
    deviceConfig.spics_io_num = m_config.pinCs;
    deviceConfig.queue_size = 1;

    esp_err_t result = spi_bus_initialize(m_config.spiHost, &busConfig, SPI_DMA_CH_AUTO);
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(m_tag, "SPI bus init failed: %s", esp_err_to_name(result));
        return false;
    }

    result = spi_bus_add_device(m_config.spiHost, &deviceConfig, &m_spi);
    if (result != ESP_OK) {
        ESP_LOGE(m_tag, "SPI device add failed: %s", esp_err_to_name(result));
        return false;
    }

    gpio_config_t intConfig = {};
    intConfig.pin_bit_mask = 1ULL << m_config.pinInt;
    intConfig.mode = GPIO_MODE_INPUT;
    intConfig.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&intConfig);

    Reset();
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t canStat = ReadRegister(RegisterCanStat);
    ESP_LOGI(m_tag, "CANSTAT after reset: 0x%02X", canStat);

    if (canStat != 0x80) {
        ESP_LOGE(m_tag, "MCP2515 SPI check failed. Expected CANSTAT 0x80");
        return false;
    }

    if (!ConfigureBitrate()) {
        return false;
    }

    if (m_config.acceptAllFrames) {
        ClearMasksAndFilters();
        WriteRegister(RegisterRxb0Ctrl, 0x60);
        WriteRegister(RegisterRxb1Ctrl, 0x60);
    }

    WriteRegister(RegisterCanInte, m_config.enableRxInterrupts ? 0x03 : 0x00);

    SetModeNormal();
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t mode = ReadRegister(RegisterCanStat) & 0xE0;
    ESP_LOGI(m_tag, "MCP2515 mode: 0x%02X", mode);

    return mode == 0x00;
}

bool Mcp2515::SendFrame(const CanFrame &frame) {
    if (frame.isExtended) {
        return SendExtended(frame.id, frame.data, frame.dlc);
    }

    return SendStandard(static_cast<uint16_t>(frame.id), frame.data, frame.dlc);
}

bool Mcp2515::SendStandard(uint16_t id, const uint8_t *data, uint8_t length) {
    if (!PrepareTxBuffer()) {
        return false;
    }

    id &= 0x07FF;
    length = ClampDlc(length);

    uint8_t tx[14] = {};
    tx[0] = CommandLoadTxb0;
    tx[1] = static_cast<uint8_t>(id >> 3);
    tx[2] = static_cast<uint8_t>((id & 0x07) << 5);
    tx[3] = 0x00;
    tx[4] = 0x00;
    tx[5] = length;

    memcpy(&tx[6], data, length);
    Transfer(tx, nullptr, 6 + length);

    RequestToSendTxb0();
    return true;
}

bool Mcp2515::SendExtended(uint32_t id, const uint8_t *data, uint8_t length) {
    if (!PrepareTxBuffer()) {
        return false;
    }

    id &= 0x1FFFFFFF;
    length = ClampDlc(length);

    uint8_t tx[14] = {};
    tx[0] = CommandLoadTxb0;
    tx[1] = static_cast<uint8_t>(id >> 21);
    tx[2] = static_cast<uint8_t>(((id >> 13) & 0xE0) | 0x08 | ((id >> 16) & 0x03));
    tx[3] = static_cast<uint8_t>(id >> 8);
    tx[4] = static_cast<uint8_t>(id);
    tx[5] = length;

    memcpy(&tx[6], data, length);
    Transfer(tx, nullptr, 6 + length);

    RequestToSendTxb0();
    return true;
}

bool Mcp2515::ReadFrame(CanFrame &frame) {
    uint8_t canIntf = ReadRegister(RegisterCanIntf);

    if (canIntf & 0x01) {
        ReadRxBuffer(CommandReadRxb0, frame);
        BitModify(RegisterCanIntf, 0x01, 0x00);
        return true;
    }

    if (canIntf & 0x02) {
        ReadRxBuffer(CommandReadRxb1, frame);
        BitModify(RegisterCanIntf, 0x02, 0x00);
        return true;
    }

    return false;
}

uint8_t Mcp2515::GetCanIntf() {
    return ReadRegister(RegisterCanIntf);
}

uint8_t Mcp2515::GetEflg() {
    return ReadRegister(RegisterEflg);
}

uint8_t Mcp2515::GetTec() {
    return ReadRegister(RegisterTec);
}

uint8_t Mcp2515::GetRec() {
    return ReadRegister(RegisterRec);
}

uint8_t Mcp2515::GetTxb0Ctrl() {
    return ReadRegister(RegisterTxb0Ctrl);
}

uint8_t Mcp2515::GetRxb0Ctrl() {
    return ReadRegister(RegisterRxb0Ctrl);
}

uint8_t Mcp2515::GetRxb1Ctrl() {
    return ReadRegister(RegisterRxb1Ctrl);
}

void Mcp2515::Transfer(const uint8_t *txData, uint8_t *rxData, size_t length) {
    spi_transaction_t transaction = {};
    transaction.length = length * 8;
    transaction.tx_buffer = txData;
    transaction.rx_buffer = rxData;

    ESP_ERROR_CHECK(spi_device_transmit(m_spi, &transaction));
}

void Mcp2515::Reset() {
    uint8_t command = CommandReset;
    Transfer(&command, nullptr, 1);
}

uint8_t Mcp2515::ReadRegister(uint8_t address) {
    uint8_t tx[3] = {CommandRead, address, 0x00};
    uint8_t rx[3] = {};
    Transfer(tx, rx, sizeof(tx));
    return rx[2];
}

void Mcp2515::WriteRegister(uint8_t address, uint8_t value) {
    uint8_t tx[3] = {CommandWrite, address, value};
    Transfer(tx, nullptr, sizeof(tx));
}

void Mcp2515::BitModify(uint8_t address, uint8_t mask, uint8_t value) {
    uint8_t tx[4] = {CommandBitModify, address, mask, value};
    Transfer(tx, nullptr, sizeof(tx));
}

void Mcp2515::SetModeNormal() {
    BitModify(RegisterCanCtrl, 0xE0, 0x00);
}

void Mcp2515::ClearMasksAndFilters() {
    for (uint8_t address = 0x00; address <= 0x0B; address++) {
        WriteRegister(address, 0x00);
    }

    for (uint8_t address = 0x10; address <= 0x1B; address++) {
        WriteRegister(address, 0x00);
    }

    for (uint8_t address = 0x20; address <= 0x27; address++) {
        WriteRegister(address, 0x00);
    }
}

bool Mcp2515::ConfigureBitrate() {
    if (m_config.oscillatorHz == 8000000 && m_config.canBitrate == 500000) {
        WriteRegister(RegisterCnf1, 0x00);
        WriteRegister(RegisterCnf2, 0x90);
        WriteRegister(RegisterCnf3, 0x82);
        return true;
    }

    ESP_LOGE(
        m_tag,
        "Unsupported bitrate/oscillator combination: bitrate=%lu oscillator=%lu",
        m_config.canBitrate,
        m_config.oscillatorHz
    );

    return false;
}

bool Mcp2515::PrepareTxBuffer() {
    uint8_t txb0ctrl = ReadRegister(RegisterTxb0Ctrl);

    if (txb0ctrl & 0x08) {
        ESP_LOGW(m_tag, "TXB0 busy, txb0ctrl=0x%02X", txb0ctrl);
        return false;
    }

    return true;
}

void Mcp2515::RequestToSendTxb0() {
    uint8_t rts = CommandRtsTxb0;
    Transfer(&rts, nullptr, 1);
}

uint8_t Mcp2515::ClampDlc(uint8_t length) {
    return length > 8 ? 8 : length;
}

void Mcp2515::ReadRxBuffer(uint8_t command, CanFrame &frame) {
    uint8_t tx[14] = {};
    uint8_t rx[14] = {};

    tx[0] = command;
    Transfer(tx, rx, sizeof(tx));

    uint8_t sidh = rx[1];
    uint8_t sidl = rx[2];
    uint8_t eid8 = rx[3];
    uint8_t eid0 = rx[4];

    frame.isExtended = (sidl & 0x08) != 0;

    if (frame.isExtended) {
        frame.id = (static_cast<uint32_t>(sidh) << 21) |
                   (static_cast<uint32_t>(sidl & 0xE0) << 13) |
                   (static_cast<uint32_t>(sidl & 0x03) << 16) |
                   (static_cast<uint32_t>(eid8) << 8) |
                   eid0;
    } else {
        frame.id = (static_cast<uint16_t>(sidh) << 3) | (sidl >> 5);
    }

    frame.dlc = rx[5] & 0x0F;
    frame.dlc = ClampDlc(frame.dlc);

    memset(frame.data, 0, sizeof(frame.data));
    memcpy(frame.data, &rx[6], frame.dlc);
}