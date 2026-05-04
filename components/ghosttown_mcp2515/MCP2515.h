#pragma once

#include <cstdint>
#include <cstddef>

#include "driver/gpio.h"
#include "driver/spi_master.h"

struct CanFrame {
    uint32_t id = 0;
    bool isExtended = false;
    uint8_t dlc = 0;
    uint8_t data[8] = {};
};

struct Mcp2515Config {
    gpio_num_t pinMosi;
    gpio_num_t pinMiso;
    gpio_num_t pinSck;
    gpio_num_t pinCs;
    gpio_num_t pinInt;

    spi_host_device_t spiHost = SPI2_HOST;
    int spiClockHz = 1 * 1000 * 1000;

    uint32_t canBitrate = 500000;
    uint32_t oscillatorHz = 8000000;

    bool acceptAllFrames = true;
    bool enableRxInterrupts = true;
};

class Mcp2515 {
public:
    explicit Mcp2515(const Mcp2515Config &config, const char *tag = "MCP2515");

    bool Init();

    bool SendFrame(const CanFrame &frame);
    bool SendStandard(uint16_t id, const uint8_t *data, uint8_t length);
    bool SendExtended(uint32_t id, const uint8_t *data, uint8_t length);

    bool ReadFrame(CanFrame &frame);

    uint8_t GetCanIntf();
    uint8_t GetEflg();
    uint8_t GetTec();
    uint8_t GetRec();
    uint8_t GetTxb0Ctrl();
    uint8_t GetRxb0Ctrl();
    uint8_t GetRxb1Ctrl();

private:
    static constexpr uint8_t CommandReset = 0xC0;
    static constexpr uint8_t CommandRead = 0x03;
    static constexpr uint8_t CommandWrite = 0x02;
    static constexpr uint8_t CommandBitModify = 0x05;
    static constexpr uint8_t CommandLoadTxb0 = 0x40;
    static constexpr uint8_t CommandRtsTxb0 = 0x81;
    static constexpr uint8_t CommandReadRxb0 = 0x90;
    static constexpr uint8_t CommandReadRxb1 = 0x94;

    static constexpr uint8_t RegisterCanStat = 0x0E;
    static constexpr uint8_t RegisterCanCtrl = 0x0F;
    static constexpr uint8_t RegisterTec = 0x1C;
    static constexpr uint8_t RegisterRec = 0x1D;
    static constexpr uint8_t RegisterCnf3 = 0x28;
    static constexpr uint8_t RegisterCnf2 = 0x29;
    static constexpr uint8_t RegisterCnf1 = 0x2A;
    static constexpr uint8_t RegisterCanInte = 0x2B;
    static constexpr uint8_t RegisterCanIntf = 0x2C;
    static constexpr uint8_t RegisterEflg = 0x2D;
    static constexpr uint8_t RegisterTxb0Ctrl = 0x30;
    static constexpr uint8_t RegisterRxb0Ctrl = 0x60;
    static constexpr uint8_t RegisterRxb1Ctrl = 0x70;

    Mcp2515Config m_config;
    const char *m_tag;
    spi_device_handle_t m_spi = nullptr;

    void Transfer(const uint8_t *txData, uint8_t *rxData, size_t length);

    void Reset();
    uint8_t ReadRegister(uint8_t address);
    void WriteRegister(uint8_t address, uint8_t value);
    void BitModify(uint8_t address, uint8_t mask, uint8_t value);

    void SetModeNormal();
    void ClearMasksAndFilters();

    bool ConfigureBitrate();
    bool PrepareTxBuffer();

    void RequestToSendTxb0();
    uint8_t ClampDlc(uint8_t length);

    void ReadRxBuffer(uint8_t command, CanFrame &frame);
};
