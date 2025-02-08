#include "State.h"

#include "hardware/flash.h"
#include "hardware/sync.h"

#include <cstring>


#define XIP_BASE 0x10000000  // Startadresse des Flash-Speichers
#define FLASH_BASE_OFFSET (1536000) // Sicherer Bereich im Flash (ab 1.5 MB)
#define FLASH_PAGE_SIZE 256          // RP2040 schreibt in 256-Byte-Seiten
#define STATE_SIZE sizeof(uint8_t)   // 1 Byte pro Zustand
#define MAX_ENTRIES (FLASH_PAGE_SIZE / STATE_SIZE) // Anzahl der möglichen Zustände pro Page


State::State(uint32_t id) : m_id(id % MAX_ENTRIES) {}


State::STATE State::GetState() {
    uint32_t lastAddress = FindLastValidEntry();
    const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + lastAddress);
    return (*flash_ptr == 1) ? OPEN : CLOSE;
}


bool State::SetState(STATE state) {
    uint32_t lastAddress = FindLastValidEntry();
    uint32_t nextAddress = lastAddress + STATE_SIZE;

    if (nextAddress >= GetFlashAddress() + FLASH_PAGE_SIZE) {
        uint32_t ints = save_and_disable_interrupts();
        flash_range_erase(GetFlashAddress(), FLASH_PAGE_SIZE);
        restore_interrupts(ints);
        nextAddress = GetFlashAddress();
    }

    uint8_t value = static_cast<uint8_t>(state);
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(nextAddress, &value, STATE_SIZE);
    restore_interrupts(ints);

    return true;
}


uint32_t State::GetFlashAddress() const {
    return FLASH_BASE_OFFSET + (m_id * FLASH_PAGE_SIZE);
}


uint32_t State::FindLastValidEntry() const {
    const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + GetFlashAddress());
    for (int i = MAX_ENTRIES - 1; i >= 0; i--) {
        if (flash_ptr[i] == 0 || flash_ptr[i] == 1) {
            return GetFlashAddress() + i;
        }
    }
    return GetFlashAddress(); // Falls nichts gefunden wurde, Anfang zurückgeben
}
