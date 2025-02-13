#include "Window.h"

#include "pico/stdlib.h"
#include "GPIO.h"


#define HIGH 1
#define LOW 0


Window::Window(
    const uint8_t pinOpen,
    const uint8_t pinClose,
    const uint32_t path,
    const uint32_t timeToOpen,
    const uint32_t timeToClose
) :
    m_pinOpen(pinOpen),
    m_pinClose(pinClose),
    m_state(State(path)),
    m_timeToOpen(timeToOpen),
    m_timeToClose(timeToClose)
{
    gpio_init(m_pinOpen);
    gpio_set_dir(m_pinOpen, GPIO_OUT);
    gpio_put(m_pinOpen, LOW);
    
    gpio_init(m_pinClose);
    gpio_set_dir(m_pinClose, GPIO_OUT);
    gpio_put(m_pinClose, LOW);
}


bool Window::Toogle() {
    if (m_state.GetState() == State::OPEN) {
        if (CloseHardware()) {
            return m_state.SetState(State::CLOSE);
        }
    }
  
    if (m_state.GetState() == State::CLOSE) {
        if (OpenHardware()) {
            return m_state.SetState(State::OPEN);
        }
    }
    
    return false;
}
  
  
bool Window::Open() {
    if (m_state.GetState() == State::OPEN) {
        if (OpenHardware()) {
            return m_state.SetState(State::OPEN);
        }
    }
  
    return false;
}
  
  
bool Window::Close() {
    if (m_state.GetState() == State::OPEN) {
        if (CloseHardware()) {
            return m_state.SetState(State::CLOSE);
        }
    }
    
    return false;
}
  
  
bool Window::ToogleState() {
    if (m_state.GetState() == State::OPEN) {
        return m_state.SetState(State::CLOSE);
    }
  
    if (m_state.GetState() == State::CLOSE) {
        return m_state.SetState(State::OPEN);
    }
  
    return false;
}
  
  
bool Window::OpenState() {
    return m_state.SetState(State::OPEN);
}
  
  
bool Window::CloseState() {
    return m_state.SetState(State::CLOSE);
}


bool Window::OpenHardware() {
    GPIO::GetInstance().AddPinToQueue(Pin(m_pinOpen, get_absolute_time(), HIGH));
    GPIO::GetInstance().AddPinToQueue(Pin(m_pinOpen, get_absolute_time() + m_timeToOpen, LOW));

    return true;
}
  
  
bool Window::CloseHardware() {
    GPIO::GetInstance().AddPinToQueue(Pin(m_pinClose, get_absolute_time(), HIGH));
    GPIO::GetInstance().AddPinToQueue(Pin(m_pinClose, get_absolute_time() + m_timeToClose, LOW));

    return true;
}
