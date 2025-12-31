#include "Window.h"

#include "pico/stdlib.h"
#include "GPIO.h"


constexpr bool high = 1;
constexpr bool low = 0;

constexpr uint8_t open = 0;
constexpr  uint8_t close = 255;


Window::Window(
	const uint8_t pinOpen,
	const uint8_t pinClose,
	const uint8_t statePos,
	const uint32_t timeToOpen,
	const uint32_t timeToClose
) :
	m_pinOpen(pinOpen),
	m_pinClose(pinClose),
	m_statePos(statePos),
	m_timeToOpen(timeToOpen),
	m_timeToClose(timeToClose
) {
	gpio_init(m_pinOpen);
	gpio_set_dir(m_pinOpen, GPIO_OUT);
	gpio_put(m_pinOpen, low);

	gpio_init(m_pinClose);
	gpio_set_dir(m_pinClose, GPIO_OUT);
	gpio_put(m_pinClose, low);
}


bool Window::Toogle() {
	switch (State::GetInstance().GetState(m_statePos)) {
		case open:
			CloseHardware();
			return State::GetInstance().SetState(close, m_statePos);
		case close:
			OpenHardware();
			return State::GetInstance().SetState(open, m_statePos);
		default:
			return false;
	}
}


bool Window::Open() {
	if (State::GetInstance().GetState(m_statePos) == close) {
		OpenHardware();
		return State::GetInstance().SetState(open, m_statePos);
	}

	return false;
}


bool Window::Close() {
	if (State::GetInstance().GetState(m_statePos) == open) {
		CloseHardware();
		return State::GetInstance().SetState(close, m_statePos);
	}

	return false;
}


bool Window::ToogleState() {
	switch (State::GetInstance().GetState(m_statePos)) {
		case open:
			return State::GetInstance().SetState(close, m_statePos);
		case close:
			return State::GetInstance().SetState(open, m_statePos);
		default:
			return false;
	}
}


bool Window::OpenState() {
	return State::GetInstance().SetState(open, m_statePos);
}


bool Window::CloseState() {
	return State::GetInstance().SetState(close, m_statePos);
}


bool Window::OpenHardware() {
	GPIO::GetInstance().AddPinToQueue(Pin(m_pinOpen, get_absolute_time(), high));
	GPIO::GetInstance().AddPinToQueue(Pin(m_pinOpen, get_absolute_time() + m_timeToOpen, low));

	return true;
}


bool Window::CloseHardware() {
	GPIO::GetInstance().AddPinToQueue(Pin(m_pinClose, get_absolute_time(), high));
	GPIO::GetInstance().AddPinToQueue(Pin(m_pinClose, get_absolute_time() + m_timeToClose, low));

	return true;
}
