#include <stdio.h>

#include "Window.h"
#include "State.h"

#include "pico/stdlib.h"


int main() {
    stdio_init_all();

    //Window leftWindow(0, 1, 0, 10000, 10000);
    //Window rightWindow(0, 1, 1, 10000, 10000);

    Window test(25, 0, 0, 2000000, 2000000);

    while (true) {
        test.Toogle();
        sleep_ms(1000);
    }
}
