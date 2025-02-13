#include "Window.h"
#include "GPIO.h"
#include "WS2812.hpp"

#include "pico/stdlib.h"
#include "pico/multicore.h"


constexpr uint8_t ledPin = 25;

constexpr uint8_t buttonLock = 8;

constexpr uint8_t buttonLeftFront = 7;
constexpr uint8_t buttonRightFront = 6;
constexpr uint8_t buttonLeftRear = 5;
constexpr uint8_t buttonRightRear = 4;

constexpr uint8_t motorLeftOpen = 3;
constexpr uint8_t motorLeftClose = 2;
constexpr uint8_t motorRightOpen = 1;
constexpr uint8_t motorRightClose = 0;

constexpr uint8_t buttonLeft = 9;
constexpr uint8_t buttonRight = 10;

constexpr uint64_t timeBetwenInterrupt = 1750000;
constexpr uint64_t timeBetwenStateInterrupt = 500000;


void SetupInPin(const uint8_t pin) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_IN);
  gpio_pull_down(pin);
}


void CallbackButtonFront(Window& window, absolute_time_t& timeLastInterrup) {
  if (absolute_time_diff_us(timeLastInterrup, get_absolute_time()) > timeBetwenInterrupt) {
    timeLastInterrup = get_absolute_time();
    window.Toogle();
  }
}


void CallbackButtonRear(Window& window, absolute_time_t& timeLastInterrup) {
  if (absolute_time_diff_us(timeLastInterrup, get_absolute_time()) > timeBetwenInterrupt) {
    if (!gpio_get(buttonLock)) {
      timeLastInterrup = get_absolute_time();
      window.Toogle();
    }
  }
}


void CallbackButton(Window& window, absolute_time_t& timeLastInterrup) {
  if (absolute_time_diff_us(timeLastInterrup, get_absolute_time()) > timeBetwenStateInterrupt) {
    timeLastInterrup = get_absolute_time();
    window.ToogleState();
  }
}


int main() {
  GPIO::MakeInstance();
  multicore_launch_core1(GPIO::Worker);

  Window leftWindow(motorLeftOpen, motorLeftClose, "/leftWindow.txt", 1500000, 1500000);
  Window rightWindow(motorRightOpen, motorRightClose, "/rightWindow.txt", 1500000, 1500000);

  absolute_time_t timeLastInterruptLeft = 0;
  absolute_time_t timeLastInterruptRight = 0;

  absolute_time_t timeLastStateInterruptLeft = 0;
  absolute_time_t timeLastStateInterruptRight = 0;

  SetupInPin(buttonLock);
  SetupInPin(buttonLeftFront);
  SetupInPin(buttonRightFront);
  SetupInPin(buttonLeftRear);
  SetupInPin(buttonRightRear);

  SetupInPin(buttonLeft);
  SetupInPin(buttonRight);

  WS2812 led(ledPin, 1, pio0, 0, WS2812::FORMAT_GRB);
  led.fill(WS2812::RGB(1, 1, 1));
  led.show();

  while (true) {
    if (gpio_get(buttonLeftFront)) {
      CallbackButtonFront(leftWindow, timeLastInterruptLeft);
    }

    if (gpio_get(buttonRightFront)) {
      CallbackButtonFront(rightWindow, timeLastInterruptRight);
    }

    if (gpio_get(buttonLeftRear)) {
      CallbackButtonRear(leftWindow, timeLastInterruptLeft);
    }

    if (gpio_get(buttonRightRear)) {
      CallbackButtonRear(rightWindow, timeLastInterruptRight);
    }

    if (gpio_get(buttonLeft)) {
      CallbackButton(leftWindow, timeLastStateInterruptLeft);
    }

    if (gpio_get(buttonRight)) {
      CallbackButton(rightWindow, timeLastStateInterruptRight);
    }
  }
}
