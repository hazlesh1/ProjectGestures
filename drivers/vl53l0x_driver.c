#include "vl53l0x_driver.h"
#include "hardware/gpio.h"
#include "pico/time.h"

static uint8_t stop_variable_left;
static uint8_t stop_variable_right;

#define VL53L0X_IO_TIMEOUT_MS 100

// helpers

static void vl53l0x_write_reg(uint8_t device_addr, uint8_t reg, uint8_t value) {
  uint8_t buffer[2] = {reg, value};
  i2c_write_blocking(I2C_PORT, device_addr, buffer, 2, false);
}

static uint8_t vl53l0x_read_reg(uint8_t device_addr, uint8_t reg) {
  uint8_t val;
  i2c_write_blocking(I2C_PORT, device_addr, &reg, 1, true);
  i2c_read_blocking(I2C_PORT, device_addr, &val, 1, false);
  return val;
}

static void vl53l0x_write_multi(uint8_t device_addr, uint8_t reg, const uint8_t *src, uint8_t count) {
  // TODO: write `count` bytes starting at `reg`, in one I2C transaction
}

static void vl53l0x_read_multi(uint8_t device_addr, uint8_t reg, uint8_t *dst, uint8_t count) {
  // TODO: read `count` bytes starting at `reg`, in one I2C transaction
}

static bool vl53l0x_poll_nonzero(uint8_t addr, uint8_t reg) {
  // TODO: poll `reg` until it reads non-zero, or timeout -> return false
}

// Datainit

static void vl53l0x_data_init(uint8_t addr, uint8_t *stop_variable_out) {
  // TODO
}

// Staticinit

static bool vl53l0x_get_spad_info(uint8_t addr, uint8_t *spad_count, bool *spad_is_aperture) {
  // TODO
}

static void vl53l0x_static_init(uint8_t addr) {
  // TODO
}

// Calibration

static bool vl53l0x_perform_single_ref_calibration(uint8_t addr, uint8_t vhv_init_byte) {
  // TODO
}

static void vl53l0x_perform_ref_calibration(uint8_t addr) {
  // TODO
}

// ---- Phase 4: Start + Read ----

static void vl53l0x_start_continuous(uint8_t addr, uint8_t stop_variable) {
  // TODO
}

// ---- Public API (declared in vl53l0x_driver.h) ----

void vl53l0x_init_bus(void) {
  i2c_init(I2C_PORT, 400 * 1000);
  gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
  gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(PIN_SDA);
  gpio_pull_up(PIN_SCL);
}

bool vl53l0x_init_sensor_array(void) {
  gpio_init(XSHUT_LEFT);
  gpio_init(XSHUT_RIGHT);
  gpio_set_dir(XSHUT_LEFT, GPIO_OUT);
  gpio_set_dir(XSHUT_RIGHT, GPIO_OUT);

  gpio_put(XSHUT_LEFT, 0);
  gpio_put(XSHUT_RIGHT, 0);
  sleep_ms(10);

  // Boot left, reassign to 0x30, then run full init sequence
  gpio_put(XSHUT_LEFT, 1);
  sleep_ms(50);
  vl53l0x_write_reg(VL53L0X_DEFAULT_ADDR, 0x8A, SENSOR_LEFT_ADDR);
  // TODO: call data_init, static_init, perform_ref_calibration, start_continuous for LEFT

  // Boot right, reassign to 0x31, then run full init sequence
  gpio_put(XSHUT_RIGHT, 1);
  sleep_ms(50);
  vl53l0x_write_reg(VL53L0X_DEFAULT_ADDR, 0x8A, SENSOR_RIGHT_ADDR);
  // TODO: call data_init, static_init, perform_ref_calibration, start_continuous for RIGHT

  return true;
}

uint16_t vl53l0x_read_distance(uint8_t addr) {
  // TODO: poll RESULT_INTERRUPT_STATUS, read range from RESULT_RANGE_STATUS+10, clear interrupt
  return VL53L0X_NOT_READY;
}