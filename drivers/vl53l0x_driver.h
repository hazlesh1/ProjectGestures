#ifndef VL53L0X_DRIVER_H
#define VL53L0X_DRIVER_H

#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>

// Hardware config (change this based on your pins for people modifying the
// code)
#define I2C_PORT i2c0
#define PIN_SDA 4
#define PIN_SCL 5
#define XSHUT_LEFT 6
#define XSHUT_RIGHT 7

// I2C address map
#define VL53L0X_DEFAULT_ADDR 0x29
#define SENSOR_LEFT_ADDR 0x30
#define SENSOR_RIGHT_ADDR 0x31

// ---- Sentinel value returned when a reading isn't ready yet ----
#define VL53L0X_NOT_READY 0xFFFF

#ifdef __cplusplus
extern "C" {
#endif

void vl53l0x_init_bus(void);
bool vl53l0x_init_sensor_array(void);
uint16_t vl53l0x_read_distance(uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif // VL53L0X_DRIVER_H