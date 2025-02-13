#include "GPIO.h"

#include "pico/stdlib.h"


#define HIGH 1
#define LOW 0


GPIO* GPIO::m_instance = nullptr;
std::vector<Pin> GPIO::m_queue;
critical_section_t GPIO::m_criticalSection;


void GPIO::MakeInstance() {
    if (m_instance == nullptr) {
        m_instance = new GPIO();
    }
}


GPIO& GPIO::GetInstance() {
    MakeInstance();
    return *m_instance;
}


void GPIO::AddPinToQueue(const Pin& pin) {
    critical_section_enter_blocking(&m_criticalSection);
    m_queue.push_back(pin);
    critical_section_exit(&m_criticalSection);
}


void GPIO::Worker() {
    while (true) {
        critical_section_enter_blocking(&m_criticalSection);
        if (!m_queue.empty()) {
            absolute_time_t currentTime = get_absolute_time();
            
            auto it = m_queue.begin();
            while (it != m_queue.end()) {
                if (currentTime >= it->m_delay) {
                    gpio_put(it->m_pin, it->m_state);
                    it = m_queue.erase(it);
                } else {
                    ++it;
                }
            }
        }
        critical_section_exit(&m_criticalSection);
    }
}



GPIO::GPIO() {
    critical_section_init(&m_criticalSection);
}


GPIO::~GPIO() {
    if (m_instance != nullptr) {
        delete m_instance;
        m_instance = nullptr;
    }
    critical_section_deinit(&m_criticalSection);
}
