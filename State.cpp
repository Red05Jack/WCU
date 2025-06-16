#include "State.h"

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"


// Flash configuration
#define FLASH_SECTOR_SIZE   4096
#define FLASH_TOTAL_SIZE    (2 * 1024 * 1024) // 2 MB Flash
#define FLASH_TARGET_OFFSET (FLASH_TOTAL_SIZE - FLASH_SECTOR_SIZE)


State* State::m_instance = nullptr;


void WriteToFlash(const uint8_t* page) {
	uint32_t ints = save_and_disable_interrupts();
	flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
	flash_range_program(FLASH_TARGET_OFFSET, page, FLASH_PAGE_SIZE);
	restore_interrupts(ints);
}


const uint8_t* ReadFromFlash() {
	return (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
}


void State::MakeInstance() {
	if (m_instance == nullptr) {
		m_instance = new State();
	}
}


State& State::GetInstance() {
	MakeInstance();
	return *m_instance;
}


// Return 0 - New State are successfully saved in flash
// Return 1 - Position too high
uint8_t State::SetState(const uint8_t value, const uint8_t position) {
	if (position > 247) {
		return 1;
	}

	m_states[position] = value;
	SaveStates();
}


uint8_t State::GetState(const uint8_t position) const {
	return m_states[position];
}



void State::SaveStates() {
	WriteToFlash(m_states);
}


void State::LoadStates() {
	const uint8_t* flash = ReadFromFlash();
	bool controlBytesCorect = true;

	for (size_t i = 0; i < FLASH_PAGE_SIZE; i++) {
		m_states[i] = flash[i];
	}

	for (size_t i = 0; i < CONTROLE_BYTES_SIZE; i++) {
		if (m_states[i + FLASH_PAGE_SIZE - CONTROLE_BYTES_SIZE] != m_controlBytes[i]) {
			controlBytesCorect = false;
		}
	}

	if (!controlBytesCorect) {
		for (size_t i = 0; i < FLASH_PAGE_SIZE; i++) {
			m_states[i] = 0;
		}

		for (size_t i = 0; i < CONTROLE_BYTES_SIZE; i++) {
			m_states[i + FLASH_PAGE_SIZE - CONTROLE_BYTES_SIZE] = m_controlBytes[i];
		}

		SaveStates();
		LoadStates();
	}
}


State::State() {
	LoadStates();
}


State::~State() {
	if (m_instance != nullptr) {
		delete m_instance;
		m_instance = nullptr;
	}
}
