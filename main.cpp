// #include "hardware/gpio.h"
// #include "hardware/i2c.h"
// #include "pico/stdio.h"
// #include "pico/time.h"
// #include <stdio.h>
// #include "drivers/vl53l0x_driver.h"

// #define I2C_PORT i2c0
// #define I2C_SDA 4
// #define I2C_SCL 5
// #define XSHUT_LEFT 6
// #define XSHUT_RIGHT 7
// #define SENSOR_LEFT_ADDR 0x30
// #define SENSOR_RIGHT_ADDR 0x29

// void vl53l0x_set_address(uint8_t old_addr, uint8_t new_addr) {
//   uint8_t data[2];

//   data[0] = 0x8A;
//   data[1] = new_addr;

//   i2c_write_blocking(I2C_PORT, old_addr, data, 2, false);
// }

// void setup_sensors() {
//   gpio_init(XSHUT_LEFT);
//   gpio_set_dir(XSHUT_LEFT, GPIO_OUT);

//   gpio_init(XSHUT_RIGHT);
//   gpio_set_dir(XSHUT_RIGHT, GPIO_OUT);

//   // offfff
//   gpio_put(XSHUT_LEFT, 0);
//   gpio_put(XSHUT_RIGHT, 0);

//   sleep_ms(10);

//   // on left
//   gpio_put(XSHUT_LEFT, 1);

//   sleep_ms(50);

//   // change sensor address
//   vl53l0x_set_address(0x29, SENSOR_LEFT_ADDR);

//   // on right
//   gpio_put(XSHUT_RIGHT, 1);

//   sleep_ms(50);
// }

// // int main() {
// //   stdio_init_all();

// //   // I2C Initialisation. Using it at 400Khz.
// //   i2c_init(I2C_PORT, 400 * 1000);

// //   gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
// //   gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
// //   gpio_pull_up(I2C_SDA);
// //   gpio_pull_up(I2C_SCL);

// //   // gpio_init(XSHUT_LEFT);
// //   // gpio_set_dir(XSHUT_LEFT, GPIO_OUT);

// //   // gpio_init(XSHUT_RIGHT);
// //   // gpio_set_dir(XSHUT_RIGHT, GPIO_OUT);

// //   // gpio_put(XSHUT_LEFT, 1);
// //   // gpio_put(XSHUT_RIGHT, 1);

// //   // For more examples of I2C use see
// //   // https://github.com/raspberrypi/pico-examples/tree/master/i2c
// //   setup_sensors();

// //   sleep_ms(2000);

// //   // vl53l0x_init(SENSOR_LEFT_ADDR);
// //   // vl53l0x_init(SENSOR_RIGHT_ADDR);

// //   while (true) {
// //     printf("Scanning...\n");

// //     for (uint8_t addr = 0x08; addr < 0x77; addr++) {

// //       uint8_t data;

// //       int result = i2c_read_blocking(I2C_PORT, addr, &data, 1, false);

// //       if (result >= 0) {
// //         printf("Found device at 0x%02X\n", addr);
// //       }
// //     }

// //     printf("----------------\n");
// //     sleep_ms(3000);
// //   }
// // }


#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
#include "drivers/vl53l0x_driver.h"

int main() {
    stdio_init_all();
    sleep_ms(2000); 

    vl53l0x_init_bus();

    vl53l0x_sensor_t sensors[2] = {
        {6, 0x30, 0}, 
        {7, 0x31, 0},  
    };

    printf("Initializing sensors...\n");
    bool ok = vl53l0x_init_sensors(sensors, 2);
    printf("Sensor init: %s\n", ok ? "OK" : "FAILED");

    while (true) {
        uint16_t left = vl53l0x_read_distance(&sensors[0]);
        uint16_t right = vl53l0x_read_distance(&sensors[1]);

        printf("Left: %u mm | Right: %u mm\n", left, right);

        sleep_ms(100);
    }
}