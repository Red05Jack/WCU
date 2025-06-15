#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>
#include <stdio.h>

// === Flash-Konfiguration ===
#define FLASH_PAGE_SIZE     256
#define FLASH_SECTOR_SIZE   4096
#define FLASH_TOTAL_SIZE    (2 * 1024 * 1024) // 2 MB Flash
#define FLASH_TARGET_OFFSET (FLASH_TOTAL_SIZE - FLASH_SECTOR_SIZE)

#define LED_PIN 25

// === Speichere ein Byte im Flash (nur erste Position der Seite) ===
void write_flag_to_flash(uint8_t value) {
    // 256-Byte-Seite vorbereiten
    uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof(page));  // Flash ist standardmäßig 0xFF (gelöscht)
    page[0] = value;                   // nur das erste Byte nutzen

    uint32_t ints = save_and_disable_interrupts();

    // Flash löschen (ein kompletter Sektor = 4096 Bytes)
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);

    // Erste Seite (256 Bytes) neu schreiben
    flash_range_program(FLASH_TARGET_OFFSET, page, FLASH_PAGE_SIZE);

    restore_interrupts(ints);
}

// === Wert aus Flash auslesen ===
uint8_t read_flag_from_flash() {
    const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    return flash_ptr[0]; // nur das erste Byte nutzen
}

int main() {
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    sleep_ms(500);  // Wartezeit bei Start für USB/Debug

    // 1. Status lesen
    uint8_t state = read_flag_from_flash();
    printf("Flash-Flag: %d\n", state);

    // 2. LED setzen basierend auf Status
    if (state == 1) {
        gpio_put(LED_PIN, 1);
    } else {
        gpio_put(LED_PIN, 0);
    }

    // 3. Status invertieren und zurück in Flash schreiben
    uint8_t new_state = (state == 1) ? 0 : 1;
    write_flag_to_flash(new_state);

  sleep_ms(5000);

    // 4. Dauerhaft warten (LED-Zustand bleibt sichtbar)
    while (true) {
         gpio_put(25, 1);
        sleep_ms(500);
        gpio_put(25, 0);
        sleep_ms(500);
    }

    return 0;
}
