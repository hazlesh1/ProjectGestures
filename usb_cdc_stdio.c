#include "pico/stdio.h"
#include "pico/stdio/driver.h"
#include "tusb.h"

static void stdio_cdc_out_chars(const char *buf, int len) {
    if (!tud_cdc_connected()) return;

    int written = 0;
    while (written < len) {
        uint32_t n = tud_cdc_write(buf + written, (uint32_t)(len - written));
        written += (int)n;
        if (n == 0) {
            tud_cdc_write_flush();
            tud_task();
        }
    }
    tud_cdc_write_flush();
}

static void stdio_cdc_out_flush(void) {
    tud_cdc_write_flush();
}

static stdio_driver_t stdio_cdc_driver = {
    .out_chars = stdio_cdc_out_chars,
    .out_flush = stdio_cdc_out_flush,
    .in_chars  = NULL,
};

void usb_cdc_stdio_init(void) {
    stdio_set_driver_enabled(&stdio_cdc_driver, true);
}