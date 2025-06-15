#pragma once

#include <cstdint>
#include <string>

class State {
public:
    enum STATE {
        CLOSE = 0,
        OPEN = 1
    };

    State(const std::string& path = "/state.dat");
    ~State() = default;

    STATE GetState() const;
    bool SetState(STATE state);

private:
    STATE m_state;
    std::string m_path;

    void load();
    void save();
};
