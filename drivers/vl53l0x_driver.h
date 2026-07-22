#ifndef VL53L0X_DRIVER_H
#define VL53L0X_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"


// Hardware Config, change this if modifying based on hardware
#define I2C_PORT i2c0
#define PIN_SDA 4
#define PIN_SCL 5

// Default address for sensors
#define VL53L0X_DEFAULT_ADDR 0x29

// Sentinel value when sensor not ready for reading
#define VL53L0X_NOT_READY 0xFFFF

// Number of sensors
#define SENSOR_COUNT 4


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    uint8_t xshut_pin;
    uint8_t i2c_addr;
    uint8_t stop_variable;
} vl53l0x_sensor_t;

void vl53l0x_init_bus(void);
bool vl53l0x_init_sensor(vl53l0x_sensor_t *sensor);
bool vl53l0x_init_sensors(vl53l0x_sensor_t *sensors, uint8_t count);
uint16_t vl53l0x_read_distance(vl53l0x_sensor_t *sensor);

#ifdef __cplusplus
}
#endif


#endif // VL53L0X_DRIVER_H