#pragma once

#include <cstdint>


#define FLASH_PAGE_SIZE     256
#define CONTROLE_BYTES_SIZE 8


class State {
public:
	State(const State& obj) = delete;


	// Public Member Methods
	static void MakeInstance();
	static State& GetInstance();

	bool SetState(const uint8_t value, const uint8_t position);
	uint8_t GetState(const uint8_t position) const;


private:
	State();
	~State();


	// Private Member Methods
	void SaveStates();
	void LoadStates();


	// Private Member Variables
	static State* m_instance;
	static uint8_t m_states[FLASH_PAGE_SIZE];
	uint8_t m_controlBytes[CONTROLE_BYTES_SIZE] = { 1,0,0,1,1,0,0,1 };


};
