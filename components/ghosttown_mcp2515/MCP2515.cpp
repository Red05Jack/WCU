#include "MCP2515.h"

#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

Mcp2515::Mcp2515(const Mcp2515Config& config, const char* tag)
  : m_config(config),
    m_tag(tag),
    m_intPin(config.pinInt, GpioMode::Input, GpioPullMode::PullUp, true) {
}

bool Mcp2515::Init() {
  if (!m_spi.InitBus(
        m_config.spiHost,
        m_config.pinMosi,
        m_config.pinMiso,
        m_config.pinSck
      )) {
    ESP_LOGE(m_tag, "SPI init failed");
    return false;
  }

  if (!m_spi.AddDevice(m_config.pinCs, m_config.spiClockHz, 0)) {
    ESP_LOGE(m_tag, "SPI device failed");
    return false;
  }

  Reset();
  vTaskDelay(pdMS_TO_TICKS(50));

  uint8_t stat = ReadRegister(RegisterCanStat);

  if (stat != 0x80) {
    ESP_LOGE(m_tag, "MCP2515 not responding");
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

  // 🔥 Interrupt aktivieren
  if (m_config.enableRxInterrupts) {
    m_intPin.AttachInterrupt(GPIO_INTR_NEGEDGE, OnInterruptStatic, this);
  }

  return true;
}

bool Mcp2515::Transfer(const uint8_t* tx, uint8_t* rx, size_t len) {
  return m_spi.Transfer(tx, rx, len);
}

void Mcp2515::Reset() {
  uint8_t cmd = CommandReset;
  Transfer(&cmd, nullptr, 1);
}

uint8_t Mcp2515::ReadRegister(uint8_t reg) {
  uint8_t tx[3] = {CommandRead, reg, 0};
  uint8_t rx[3] = {};
  Transfer(tx, rx, 3);
  return rx[2];
}

void Mcp2515::WriteRegister(uint8_t reg, uint8_t value) {
  uint8_t tx[3] = {CommandWrite, reg, value};
  Transfer(tx, nullptr, 3);
}

void Mcp2515::BitModify(uint8_t reg, uint8_t mask, uint8_t value) {
  uint8_t tx[4] = {CommandBitModify, reg, mask, value};
  Transfer(tx, nullptr, 4);
}

void Mcp2515::SetModeNormal() {
  BitModify(RegisterCanCtrl, 0xE0, 0x00);
}

void Mcp2515::ClearMasksAndFilters() {
  for (uint8_t i = 0; i < 0x2F; i++) {
    WriteRegister(i, 0);
  }
}

bool Mcp2515::ConfigureBitrate() {
  if (m_config.oscillatorHz == 8000000 && m_config.canBitrate == 500000) {
    WriteRegister(RegisterCnf1, 0x00);
    WriteRegister(RegisterCnf2, 0x90);
    WriteRegister(RegisterCnf3, 0x82);
    return true;
  }
  return false;
}

bool Mcp2515::PrepareTxBuffer() {
  uint8_t ctrl = ReadRegister(RegisterTxb0Ctrl);
  return !(ctrl & 0x08);
}

void Mcp2515::RequestToSendTxb0() {
  uint8_t cmd = CommandRtsTxb0;
  Transfer(&cmd, nullptr, 1);
}

uint8_t Mcp2515::ClampDlc(uint8_t len) {
  return len > 8 ? 8 : len;
}

bool Mcp2515::SendFrame(const CanFrame& frame) {
  if (frame.isExtended) {
    return SendExtended(frame.id, frame.data, frame.dlc);
  }
  return SendStandard(frame.id, frame.data, frame.dlc);
}

bool Mcp2515::SendStandard(uint16_t id, const uint8_t* data, uint8_t len) {
  if (!PrepareTxBuffer()) return false;

  len = ClampDlc(len);

  uint8_t tx[14] = {};
  tx[0] = CommandLoadTxb0;
  tx[1] = id >> 3;
  tx[2] = (id & 0x07) << 5;
  tx[5] = len;

  memcpy(&tx[6], data, len);

  Transfer(tx, nullptr, 6 + len);
  RequestToSendTxb0();
  return true;
}

bool Mcp2515::SendExtended(uint32_t id, const uint8_t* data, uint8_t len) {
  if (!PrepareTxBuffer()) return false;

  len = ClampDlc(len);

  uint8_t tx[14] = {};
  tx[0] = CommandLoadTxb0;

  tx[1] = id >> 21;
  tx[2] = ((id >> 13) & 0xE0) | 0x08 | ((id >> 16) & 0x03);
  tx[3] = id >> 8;
  tx[4] = id;
  tx[5] = len;

  memcpy(&tx[6], data, len);

  Transfer(tx, nullptr, 6 + len);
  RequestToSendTxb0();
  return true;
}

bool Mcp2515::ReadFrame(CanFrame& frame) {
  uint8_t intf = ReadRegister(RegisterCanIntf);

  if (intf & 0x01) {
    ReadRxBuffer(CommandReadRxb0, frame);
    BitModify(RegisterCanIntf, 0x01, 0);
    return true;
  }

  if (intf & 0x02) {
    ReadRxBuffer(CommandReadRxb1, frame);
    BitModify(RegisterCanIntf, 0x02, 0);
    return true;
  }

  return false;
}

void Mcp2515::ReadRxBuffer(uint8_t cmd, CanFrame& frame) {
  uint8_t tx[14] = {};
  uint8_t rx[14] = {};

  tx[0] = cmd;
  Transfer(tx, rx, 14);

  uint8_t sidh = rx[1];
  uint8_t sidl = rx[2];

  frame.isExtended = sidl & 0x08;

  if (frame.isExtended) {
    frame.id =
      ((uint32_t)sidh << 21) |
      ((uint32_t)(sidl & 0xE0) << 13) |
      ((uint32_t)(sidl & 0x03) << 16) |
      ((uint32_t)rx[3] << 8) |
      rx[4];
  } else {
    frame.id = (sidh << 3) | (sidl >> 5);
  }

  frame.dlc = ClampDlc(rx[5] & 0x0F);
  memcpy(frame.data, &rx[6], frame.dlc);
}

void Mcp2515::OnInterruptStatic(void* arg) {
  reinterpret_cast<Mcp2515*>(arg)->OnInterrupt();
}

void Mcp2515::OnInterrupt() {
  CanFrame frame;

  while (ReadFrame(frame)) {
    ESP_LOGI(m_tag, "CAN RX ID: 0x%lX DLC:%d", frame.id, frame.dlc);
  }
}