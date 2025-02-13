#include "Window.h"
#include "GPIO.h"

#include "pico/stdlib.h"
#include "pico/multicore.h"


#define LED 25

#define LOCK 8
#define BUTTON_LEFT_FRONT 7
#define BUTTON_RIGHT_FRONT 6
#define BUTTON_LEFT_REAR 5
#define BUTTON_RIGHT_REAR 4

#define MOTOR_LEFT_OPEN 3
#define MOTOR_LEFT_CLOSE 2
#define MOTOR_RIGHT_OPEN 1
#define MOTOR_RIGHT_CLOSE 0

#define BUTTON_LEFT 9
#define BUTTON_RIGHT 10

#define TIME_BETWEN_INTERRUPT 1750000
#define TIME_BETWEN_STATE_INTERRUPT 500000


void SetupInPin(const uint8_t pin) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_IN);
  gpio_pull_down(pin);
}


void CallbackButtonFront(Window& window, absolute_time_t& timeLastInterrup) {
  if (absolute_time_diff_us(timeLastInterrup, get_absolute_time()) > TIME_BETWEN_INTERRUPT) {
    timeLastInterrup = get_absolute_time();
    window.Toogle();
  }
}


void CallbackButtonRear(Window& window, absolute_time_t& timeLastInterrup) {
  if (absolute_time_diff_us(timeLastInterrup, get_absolute_time()) > TIME_BETWEN_INTERRUPT) {
    if (!gpio_get(LOCK)) {
      timeLastInterrup = get_absolute_time();
      window.Toogle();
    }
  }
}


void CallbackButton(Window& window, absolute_time_t& timeLastInterrup) {
  if (absolute_time_diff_us(timeLastInterrup, get_absolute_time()) > TIME_BETWEN_STATE_INTERRUPT) {
    timeLastInterrup = get_absolute_time();
    window.ToogleState();
  }
}


int main() {
  GPIO::MakeInstance();
  multicore_launch_core1(GPIO::Worker);

  Window leftWindow(MOTOR_LEFT_OPEN, MOTOR_LEFT_CLOSE, "/leftWindow.txt", 1500000, 1500000);
  Window rightWindow(MOTOR_RIGHT_OPEN, MOTOR_RIGHT_CLOSE, "/rightWindow.txt", 1500000, 1500000);

  absolute_time_t timeLastInterruptLeft = 0;
  absolute_time_t timeLastInterruptRight = 0;

  absolute_time_t timeLastStateInterruptLeft = 0;
  absolute_time_t timeLastStateInterruptRight = 0;

  SetupInPin(LOCK);
  SetupInPin(BUTTON_LEFT_FRONT);
  SetupInPin(BUTTON_RIGHT_FRONT);
  SetupInPin(BUTTON_LEFT_REAR);
  SetupInPin(BUTTON_RIGHT_REAR);

  SetupInPin(BUTTON_LEFT);
  SetupInPin(BUTTON_RIGHT);

  while (true) {
    if (gpio_get(BUTTON_LEFT_FRONT)) {
      CallbackButtonFront(leftWindow, timeLastInterruptLeft);
    }

    if (gpio_get(BUTTON_RIGHT_FRONT)) {
      CallbackButtonFront(rightWindow, timeLastInterruptRight);
    }

    if (gpio_get(BUTTON_LEFT_REAR)) {
      CallbackButtonRear(leftWindow, timeLastInterruptLeft);
    }

    if (gpio_get(BUTTON_RIGHT_REAR)) {
      CallbackButtonRear(rightWindow, timeLastInterruptRight);
    }

    if (gpio_get(BUTTON_LEFT)) {
      CallbackButton(leftWindow, timeLastStateInterruptLeft);
    }

    if (gpio_get(BUTTON_RIGHT)) {
      CallbackButton(rightWindow, timeLastStateInterruptRight);
    }
  }
}
