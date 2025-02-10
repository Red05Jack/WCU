#include <stdio.h>

#include "Window.h"

#include "pico/stdlib.h"


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

#define TIME_BETWEN_INTERRUPT_LEFT 1000
#define TIME_BETWEN_INTERRUPT_RIGHT 1000


absolute_time_t timeLastInterruptLeft = 0;
absolute_time_t timeLastInterruptRight = 0;

Window leftWindow(MOTOR_LEFT_OPEN, MOTOR_LEFT_CLOSE, 0, 1000, 1000);
Window rightWindow(MOTOR_RIGHT_OPEN, MOTOR_RIGHT_CLOSE, 1, 1000, 1000);


void SetupInPin(const uint8_t pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_down(pin);
}


void CallbackButtonLeftFront(uint gpio, uint32_t events) {
    if (absolute_time_diff_us(timeLastInterruptLeft, get_absolute_time()) > TIME_BETWEN_INTERRUPT_LEFT) {
        timeLastInterruptLeft = get_absolute_time();
        leftWindow.Toogle();
    }
}


void CallbackButtonRightFront(uint gpio, uint32_t events) {
    if (absolute_time_diff_us(timeLastInterruptRight, get_absolute_time()) > TIME_BETWEN_INTERRUPT_RIGHT) {
        timeLastInterruptRight = get_absolute_time();
        rightWindow.Toogle();
    }
}


void CallbackButtonLeftRear(uint gpio, uint32_t events) {
    if (absolute_time_diff_us(timeLastInterruptLeft, get_absolute_time()) > TIME_BETWEN_INTERRUPT_LEFT) {
        if (!gpio_get(LOCK)) {
            timeLastInterruptLeft = get_absolute_time();
            leftWindow.Toogle();
        }
    }
}


void CallbackButtonRightRear(uint gpio, uint32_t events) {
    if (absolute_time_diff_us(timeLastInterruptRight, get_absolute_time()) > TIME_BETWEN_INTERRUPT_RIGHT) {
        if (!gpio_get(LOCK)) {
            timeLastInterruptRight = get_absolute_time();
            rightWindow.Toogle();
        }
    }
}


void CallbackButtonLeft(uint gpio, uint32_t events) {
    leftWindow.ToogleState();
}


void CallbackButtonRight(uint gpio, uint32_t events) {
    rightWindow.ToogleState();
}


int main() {
    SetupInPin(LOCK);
    SetupInPin(BUTTON_LEFT_FRONT);
    SetupInPin(BUTTON_RIGHT_FRONT);
    SetupInPin(BUTTON_LEFT_REAR);
    SetupInPin(BUTTON_RIGHT_REAR);

    gpio_set_irq_enabled_with_callback(BUTTON_LEFT_FRONT, GPIO_IRQ_EDGE_RISE, true, &CallbackButtonLeftFront);
    gpio_set_irq_enabled_with_callback(BUTTON_RIGHT_FRONT, GPIO_IRQ_EDGE_RISE, true, &CallbackButtonRightFront);
    gpio_set_irq_enabled_with_callback(BUTTON_LEFT_REAR, GPIO_IRQ_EDGE_RISE, true, &CallbackButtonLeftRear);
    gpio_set_irq_enabled_with_callback(BUTTON_RIGHT_REAR, GPIO_IRQ_EDGE_RISE, true, &CallbackButtonRightRear);

    SetupInPin(BUTTON_LEFT);
    SetupInPin(BUTTON_RIGHT);

    gpio_set_irq_enabled_with_callback(BUTTON_LEFT, GPIO_IRQ_EDGE_RISE, true, &CallbackButtonLeft);
    gpio_set_irq_enabled_with_callback(BUTTON_RIGHT_REAR, GPIO_IRQ_EDGE_RISE, true, &CallbackButtonRight);

    while (true) {
        sleep_ms(1000);
    }
}
