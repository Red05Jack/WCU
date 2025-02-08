#pragma once

#include <cstdint>


class State {
public:
    State(uint32_t id);
    ~State() = default;


    // Public Member Enums
    enum STATE {
        OPEN = 1,
        CLOSE = 0
    };


    // Public Member Methods
    STATE GetState();
    bool SetState(STATE state);


private:
    // Private Member Variables
    uint32_t m_id;


    // Private Member Methods
    uint32_t GetFlashAddress() const;
    uint32_t FindLastValidEntry() const;


};
