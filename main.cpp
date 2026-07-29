#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
#include "drivers/vl53l0x_driver.h"
#include "gesture/gesture_logic.h"
#include "hid/hid_output.h"
#include "usb_cdc_stdio.h"

#define SENSOR_POLL_INTERVAL_MS 25

#define LED_GESTURE_FIRED_PIN 17  
#define LED_CYCLE_START_PIN   16  
#define LED_CYCLE_ACTIVE_PIN  18  

#define CYCLE_START_PULSE_MS   10 
#define GESTURE_PULSE_MS      150  

int main() {
    stdio_init_all();
    sleep_ms(2000);

    gpio_init(LED_GESTURE_FIRED_PIN);
    gpio_set_dir(LED_GESTURE_FIRED_PIN, GPIO_OUT);
    gpio_put(LED_GESTURE_FIRED_PIN, 0);

    gpio_init(LED_CYCLE_START_PIN);
    gpio_set_dir(LED_CYCLE_START_PIN, GPIO_OUT);
    gpio_put(LED_CYCLE_START_PIN, 0);

    gpio_init(LED_CYCLE_ACTIVE_PIN);
    gpio_set_dir(LED_CYCLE_ACTIVE_PIN, GPIO_OUT);
    gpio_put(LED_CYCLE_ACTIVE_PIN, 0);

    vl53l0x_init_bus();

    vl53l0x_sensor_t sensors[SENSOR_COUNT] = {
        {6, 0x30, 0},  
        {7, 0x31, 0}, 
        {8, 0x32, 0},  
        {9, 0x33, 0}, 
    };

    bool ok = vl53l0x_init_sensors(sensors, SENSOR_COUNT);

    gesture_logic_init();
    hid_output_init();
    usb_cdc_stdio_init();
    sleep_ms(3000);
    printf("Sensor init: %s\n", ok ? "OK" : "FAILED");

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    uint32_t next_poll_ms       = now_ms;
    uint32_t cycle_start_off_ms = 0;
    uint32_t gesture_led_off_ms = 0;

    while (true) {
        hid_output_task();

        now_ms = to_ms_since_boot(get_absolute_time());

        if (cycle_start_off_ms != 0 && now_ms >= cycle_start_off_ms) {
            gpio_put(LED_CYCLE_START_PIN, 0);
            cycle_start_off_ms = 0;
        }

        if (gesture_led_off_ms != 0 && now_ms >= gesture_led_off_ms) {
            gpio_put(LED_GESTURE_FIRED_PIN, 0);
            gesture_led_off_ms = 0;
        }

        if (now_ms >= next_poll_ms) {
            next_poll_ms = now_ms + SENSOR_POLL_INTERVAL_MS;

            gpio_put(LED_CYCLE_ACTIVE_PIN, 1);

            gpio_put(LED_CYCLE_START_PIN, 1);
            cycle_start_off_ms = now_ms + CYCLE_START_PULSE_MS;

            uint16_t left  = vl53l0x_read_distance(&sensors[0]);
            uint16_t right = vl53l0x_read_distance(&sensors[1]);
            uint16_t up    = vl53l0x_read_distance(&sensors[2]);
            uint16_t down  = vl53l0x_read_distance(&sensors[3]);

            gesture_event_t gesture = gesture_logic_update(left, right, up, down, now_ms);

            if (gesture != GESTURE_NONE) {
                const char *label = "";
                switch (gesture) {
                    case GESTURE_SWIPE_LEFT:  label = "SWIPE LEFT";  break;
                    case GESTURE_SWIPE_RIGHT: label = "SWIPE RIGHT"; break;
                    case GESTURE_SWIPE_UP:    label = "SWIPE UP";    break;
                    case GESTURE_SWIPE_DOWN:  label = "SWIPE DOWN"; break;
                    case GESTURE_HOVER:       label = "HOVER";       break;
                    default: break;
                }
                printf(">>> GESTURE: %s <<<\n", label);
                hid_output_dispatch(gesture);

                gpio_put(LED_GESTURE_FIRED_PIN, 1);
                gesture_led_off_ms = now_ms + GESTURE_PULSE_MS;
            }
            gpio_put(LED_CYCLE_ACTIVE_PIN, 0);
        }
    }
}