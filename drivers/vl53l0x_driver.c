#include "vl53l0x_driver.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>
#define VL53L0X_IO_TIMEOUT_MS 100

// Used: Documentation, Pololu VL53L0X Arduino C++ library (THANKS GUYS, some things are directly translated), VScode to fix syntax errors
// Configs, tuning and some calibrations are copied from the documentation, so if it's not working, I'm blaming the docs LMAO 
// Apologies for the comments, I was in a lot of pain trying to understand this sensor

// MAGIC NUMBERS, NAMED BECAUSE WANDERING AROUND THE DOCUMENTATION PAGE WONDERING WHAT 0x83 MEANS IS NOT THE HAPPIEST DAYS


// bank switch/page select registers
#define PAGE_SELECT       0xFF   // switch between register banks
#define POWER_SETUP       0x80   // 0x01 = enter setup mode, 0x00 = exit
#define SYSRANGE_START    0x00   // trigger a measurement/continuous mode
#define STOP_VAR_REG      0x91   // stop_variable register

// ranging control/status
#define SEQUENCE_CONFIG   0x01   // what measurement stages are enabled
#define INTERRUPT_STATUS  0x13   // check reading ready or not
#define INTERRUPT_CLEAR   0x0B   // write 0x01 here to clear pending interrupt
#define RANGE_RESULT_HI   0x1E   // high byte of the distance result

// signal quality
#define MSRC_CONFIG       0x60
#define SIGNAL_RATE_LIMIT 0x44

// -- SPAD calibration registers (I have absolutely not idea what this is) -- 
#define SPAD_INFO_CTRL86  0x83   // used for both status flag and data read
#define SPAD_INFO_TRIGGER 0x94
#define SPAD_INFO_RESULT  0x92
#define SPAD_MAP_START    0xB0   // GLOBAL_CONFIG_SPAD_ENABLES_REF_0, 6 bytes 
#define SPAD_REF_START    0xB6
#define SPAD_NUM_REQ_REF  0x4E
#define SPAD_REF_EN_OFF   0x4F

// -- calibration bytes for VHV/phase steps (this is just from the documentation, no idea again) --
#define CAL_VHV_INIT_BYTE   0x40
#define CAL_PHASE_INIT_BYTE 0x00
#define SEQ_STEP_VHV        0x01
#define SEQ_STEP_PHASE      0x02
#define SEQ_STEP_NORMAL     0xE8 // normal operation sequence config, restored after calibration

#define I2C_ADDR_CHANGE_REG 0x8A // I2C_SLAVE_DEVICE_ADDRESS


// Write
static void vl53l0x_write_reg(uint8_t device_addr, uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, device_addr, buffer, 2, false);
}


// Read
static uint8_t vl53l0x_read_reg(uint8_t device_addr, uint8_t reg)
{
    uint8_t value;
    i2c_write_blocking(I2C_PORT, device_addr, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, device_addr, &value, 1, false);
    return value;
}


// Load VL53L0X tuning settings (copy pasted from documentation btw)
static void vl53l0x_load_tuning_settings(vl53l0x_sensor_t *sensor)
{
    uint8_t addr = sensor->i2c_addr;

    vl53l0x_write_reg(addr, 0x88, 0x00);
    vl53l0x_write_reg(addr, POWER_SETUP, 0x01);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x00);

    sensor->stop_variable = vl53l0x_read_reg(addr, STOP_VAR_REG); // needed to store value apparently 

    vl53l0x_write_reg(addr, SYSRANGE_START, 0x01);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, POWER_SETUP, 0x00);
}

// for start measuring
// static void vl53l0x_start_ranging(vl53l0x_sensor_t *sensor)
// {
//     uint8_t addr = sensor->i2c_addr;

//     vl53l0x_write_reg(addr, POWER_SETUP, 0x01);
//     vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
//     vl53l0x_write_reg(addr, SYSRANGE_START, 0x00);
//     vl53l0x_write_reg(addr, STOP_VAR_REG, sensor->stop_variable); 
//     vl53l0x_write_reg(addr, SYSRANGE_START, 0x01);
//     vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
//     vl53l0x_write_reg(addr, POWER_SETUP, 0x00);

//     vl53l0x_write_reg(addr, SYSRANGE_START, 0x02); 
// }

// static void vl53l0x_start_ranging(vl53l0x_sensor_t *sensor){
//   uint8_t addr = sensor->i2c_addr;
//   vl53l0x_write_reg(addr, SYSRANGE_START, 0x01);
// }

static void vl53l0x_start_ranging(vl53l0x_sensor_t *sensor)
{
    uint8_t addr = sensor->i2c_addr;
    vl53l0x_write_reg(addr, POWER_SETUP, 0x01);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x00);
    vl53l0x_write_reg(addr, STOP_VAR_REG, sensor->stop_variable);
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x01);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, POWER_SETUP, 0x00);
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x02);
}


// Initialise I2C bus (default settings)
void vl53l0x_init_bus(void)
{
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);

    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);
}

uint16_t vl53l0x_read_distance(vl53l0x_sensor_t *sensor)
{
    uint8_t addr = sensor->i2c_addr;

    // Check measurement ready or nah
    if ((vl53l0x_read_reg(addr, INTERRUPT_STATUS) & 0x07) == 0)
    {
        return VL53L0X_NOT_READY;
    }

    uint8_t reg = RANGE_RESULT_HI;
    uint8_t data[2];
    i2c_write_blocking(I2C_PORT, addr, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, addr, data, 2, false);
    vl53l0x_write_reg(addr, INTERRUPT_CLEAR, 0x01);
    return ((uint16_t)data[0] << 8) | data[1];
}


static bool vl53l0x_poll_nonzero(uint8_t addr, uint8_t reg)
{
    absolute_time_t deadline = make_timeout_time_ms(VL53L0X_IO_TIMEOUT_MS);

    while (!time_reached(deadline)) {
        if (vl53l0x_read_reg(addr, reg) != 0x00) {
            return true;
        }
        sleep_ms(1);
    }
    return false;
}

static bool vl53l0x_get_spad_info(uint8_t addr, uint8_t *spad_count, bool *spad_is_aperture)
{
    uint8_t tmp;

    vl53l0x_write_reg(addr, POWER_SETUP, 0x01);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x00);

    vl53l0x_write_reg(addr, PAGE_SELECT, 0x06);
    tmp = vl53l0x_read_reg(addr, SPAD_INFO_CTRL86);
    vl53l0x_write_reg(addr, SPAD_INFO_CTRL86, tmp | 0x04);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x07);
    vl53l0x_write_reg(addr, 0x81, 0x01);

    vl53l0x_write_reg(addr, POWER_SETUP, 0x01);

    vl53l0x_write_reg(addr, SPAD_INFO_TRIGGER, 0x6b);
    vl53l0x_write_reg(addr, SPAD_INFO_CTRL86, 0x00);

    if (!vl53l0x_poll_nonzero(addr, SPAD_INFO_CTRL86)) {
        return false; // time out > basically comms or sensor problem 
    }

    vl53l0x_write_reg(addr, SPAD_INFO_CTRL86, 0x01);
    tmp = vl53l0x_read_reg(addr, SPAD_INFO_RESULT);

    *spad_count = tmp & 0x7f;
    *spad_is_aperture = (tmp >> 7) & 0x01;

    vl53l0x_write_reg(addr, 0x81, 0x00);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x06);
    tmp = vl53l0x_read_reg(addr, SPAD_INFO_CTRL86);
    vl53l0x_write_reg(addr, SPAD_INFO_CTRL86, tmp & ~0x04);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x01);

    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, POWER_SETUP, 0x00);

    return true;
}

static void vl53l0x_set_spads_and_tuning(uint8_t addr, uint8_t spad_count, bool spad_is_aperture)
{
    uint8_t ref_spad_map[6];
    uint8_t reg = SPAD_MAP_START; 
    i2c_write_blocking(I2C_PORT, addr, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, addr, ref_spad_map, 6, false);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, SPAD_REF_EN_OFF, 0x00);
    vl53l0x_write_reg(addr, SPAD_NUM_REQ_REF, 0x2C);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, SPAD_REF_START, 0xB4);
    uint8_t first_spad_to_enable = spad_is_aperture ? 12 : 0;
    uint8_t spads_enabled = 0;
    uint8_t new_spad_map[6] = {0, 0, 0, 0, 0, 0};

    for (uint8_t i = 0; i < 48; i++) {
        if (i < first_spad_to_enable || spads_enabled >= spad_count) {
        } else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x01) {
            new_spad_map[i / 8] |= (1 << (i % 8));
            spads_enabled++;
        }
    }

    reg = SPAD_MAP_START;
    uint8_t buf[7] = {reg, new_spad_map[0], new_spad_map[1], new_spad_map[2],
                       new_spad_map[3], new_spad_map[4], new_spad_map[5]};
    i2c_write_blocking(I2C_PORT, addr, buf, 7, false);

    // Default tuning settings (PRAY THIS WORKS AHHHHHH, from documentation btw)
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x00);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, 0x09, 0x00);
    vl53l0x_write_reg(addr, 0x10, 0x00);
    vl53l0x_write_reg(addr, 0x11, 0x00);
    vl53l0x_write_reg(addr, 0x24, 0x01);
    vl53l0x_write_reg(addr, 0x25, 0xFF);
    vl53l0x_write_reg(addr, 0x75, 0x00);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, SPAD_NUM_REQ_REF, 0x2C);
    vl53l0x_write_reg(addr, 0x48, 0x00);
    vl53l0x_write_reg(addr, 0x30, 0x20);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, 0x30, 0x09);
    vl53l0x_write_reg(addr, 0x54, 0x00);
    vl53l0x_write_reg(addr, 0x31, 0x04);
    vl53l0x_write_reg(addr, 0x32, 0x03);
    vl53l0x_write_reg(addr, 0x40, 0x83);
    vl53l0x_write_reg(addr, 0x46, 0x25);
    vl53l0x_write_reg(addr, MSRC_CONFIG, 0x00);
    vl53l0x_write_reg(addr, 0x27, 0x00);
    vl53l0x_write_reg(addr, 0x50, 0x06);
    vl53l0x_write_reg(addr, 0x51, 0x00);
    vl53l0x_write_reg(addr, 0x52, 0x96);
    vl53l0x_write_reg(addr, 0x56, 0x08);
    vl53l0x_write_reg(addr, 0x57, 0x30);
    vl53l0x_write_reg(addr, 0x61, 0x00);
    vl53l0x_write_reg(addr, 0x62, 0x00);
    vl53l0x_write_reg(addr, 0x64, 0x00);
    vl53l0x_write_reg(addr, 0x65, 0x00);
    vl53l0x_write_reg(addr, 0x66, 0xA0);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, 0x22, 0x32);
    vl53l0x_write_reg(addr, 0x47, 0x14);
    vl53l0x_write_reg(addr, 0x49, 0xFF);
    vl53l0x_write_reg(addr, 0x4A, 0x00);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, 0x7A, 0x0A);
    vl53l0x_write_reg(addr, 0x7B, 0x00);
    vl53l0x_write_reg(addr, 0x78, 0x21);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, 0x23, 0x34);
    vl53l0x_write_reg(addr, 0x42, 0x00);
    vl53l0x_write_reg(addr, 0x44, 0xFF);
    vl53l0x_write_reg(addr, 0x45, 0x26);
    vl53l0x_write_reg(addr, 0x46, 0x05);
    vl53l0x_write_reg(addr, 0x40, 0x40);
    vl53l0x_write_reg(addr, 0x0E, 0x06);
    vl53l0x_write_reg(addr, 0x20, 0x1A);
    vl53l0x_write_reg(addr, 0x43, 0x40);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, 0x34, 0x03);
    vl53l0x_write_reg(addr, 0x35, 0x44);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, 0x31, 0x04);
    vl53l0x_write_reg(addr, 0x4B, 0x09);
    vl53l0x_write_reg(addr, 0x4C, 0x05);
    vl53l0x_write_reg(addr, 0x4D, 0x04);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, SIGNAL_RATE_LIMIT, 0x00);
    vl53l0x_write_reg(addr, 0x45, 0x20);
    vl53l0x_write_reg(addr, 0x47, 0x08);
    vl53l0x_write_reg(addr, 0x48, 0x28);
    vl53l0x_write_reg(addr, 0x67, 0x00);
    vl53l0x_write_reg(addr, 0x70, 0x04);
    vl53l0x_write_reg(addr, 0x71, 0x01);
    vl53l0x_write_reg(addr, 0x72, 0xFE);
    vl53l0x_write_reg(addr, 0x76, 0x00);
    vl53l0x_write_reg(addr, 0x77, 0x00);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, 0x0D, 0x01);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, POWER_SETUP, 0x01);
    vl53l0x_write_reg(addr, 0x01, 0xF8);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, 0x8E, 0x01);
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x01);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, POWER_SETUP, 0x00);
}

static void vl53l0x_configure_interrupt_and_sequence(uint8_t addr)
{
    uint8_t msrc_config = vl53l0x_read_reg(addr, MSRC_CONFIG); 
    vl53l0x_write_reg(addr, MSRC_CONFIG, msrc_config | 0x12);
    uint16_t signal_rate_limit_fixed = (uint16_t)(0.25f * 128.0f); 
    uint8_t buf[3] = {SIGNAL_RATE_LIMIT, (uint8_t)(signal_rate_limit_fixed >> 8), (uint8_t)(signal_rate_limit_fixed & 0xFF)};
    i2c_write_blocking(I2C_PORT, addr, buf, 3, false);
    vl53l0x_write_reg(addr, SEQUENCE_CONFIG, 0xFF); 
}

static bool vl53l0x_perform_single_ref_calibration(uint8_t addr, uint8_t vhv_init_byte)
{
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x01 | vhv_init_byte); 
    printf("  wrote SYSRANGE_START=0x%02X, polling...\n", 0x01 | vhv_init_byte);

    uint8_t readback = vl53l0x_read_reg(addr, SYSRANGE_START);
    printf("  SYSRANGE_START readback = 0x%02X\n", readback);

    absolute_time_t deadline = make_timeout_time_ms(VL53L0X_IO_TIMEOUT_MS);
    while ((vl53l0x_read_reg(addr, INTERRUPT_STATUS) & 0x07) == 0) { 
        if (time_reached(deadline)) {
            uint8_t final_status = vl53l0x_read_reg(addr, INTERRUPT_STATUS);
            printf("  TIMEOUT: INTERRUPT_STATUS stuck at 0x%02X\n", final_status);
            return false; 
        }
    }

    vl53l0x_write_reg(addr, INTERRUPT_CLEAR, 0x01); 
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x00); 
    return true;
}

static bool vl53l0x_perform_ref_calibration(uint8_t addr)
{
    // VHV calibration
    vl53l0x_write_reg(addr, SEQUENCE_CONFIG, SEQ_STEP_VHV);
    uint8_t seq_check = vl53l0x_read_reg(addr, SEQUENCE_CONFIG);
    
    if (!vl53l0x_perform_single_ref_calibration(addr, CAL_VHV_INIT_BYTE)) {
        printf("FAILED at VHV calibration (addr 0x%02X)\n", addr);
        return false;
    }
    printf("VHV calibration OK (addr 0x%02X)\n", addr);

    // Phase calibration
    vl53l0x_write_reg(addr, SEQUENCE_CONFIG, SEQ_STEP_PHASE);
    if (!vl53l0x_perform_single_ref_calibration(addr, CAL_PHASE_INIT_BYTE)) {
        printf("FAILED at phase calibration (addr 0x%02X)\n", addr);
        return false;
    }
    printf("phase calibration OK (addr 0x%02X)\n", addr);

    // Restore the sequence config to the normal operating value
    vl53l0x_write_reg(addr, SEQUENCE_CONFIG, SEQ_STEP_NORMAL);

    return true;
}

// ============================================================================
// CHANGED SOME STUFF HERE: vl53l0x_init_sensor now calls the FULL init sequence instead. Had some help online to fix errors ;-; 
//   1. load_tuning_settings   
//   2. configure_interrupt_and_sequence  
//   3. get_spad_info          
//   4. set_spads_and_tuning   
//   5. perform_ref_calibration 
//   6. start_ranging          
// ============================================================================
bool vl53l0x_init_sensor(vl53l0x_sensor_t *sensor)
{
    uint8_t addr = sensor->i2c_addr;

    gpio_put(sensor->xshut_pin, 1);
    sleep_ms(50);

    vl53l0x_write_reg(VL53L0X_DEFAULT_ADDR, I2C_ADDR_CHANGE_REG, addr);
    sleep_ms(10);

    printf("Sensor 0x%02X\n", addr);
    printf("MODEL_ID = %02X\n", vl53l0x_read_reg(addr, 0xC0));
    printf("REVISION = %02X\n", vl53l0x_read_reg(addr, 0xC2));
    vl53l0x_write_reg(addr, 0x88, 0x00);

    vl53l0x_write_reg(addr, POWER_SETUP, 0x01);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x01);
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x00);
    sensor->stop_variable = vl53l0x_read_reg(addr, STOP_VAR_REG);
    vl53l0x_write_reg(addr, SYSRANGE_START, 0x01);
    vl53l0x_write_reg(addr, PAGE_SELECT, 0x00);
    vl53l0x_write_reg(addr, POWER_SETUP, 0x00);

    uint8_t msrc = vl53l0x_read_reg(addr, MSRC_CONFIG);
    vl53l0x_write_reg(addr, MSRC_CONFIG, msrc | 0x12);
    uint8_t srl[3] = { SIGNAL_RATE_LIMIT, 0x00, 0x20 };
    i2c_write_blocking(I2C_PORT, addr, srl, 3, false);

    vl53l0x_write_reg(addr, SEQUENCE_CONFIG, 0xFF);

    uint8_t spad_count;
    bool spad_is_aperture;
    if (!vl53l0x_get_spad_info(addr, &spad_count, &spad_is_aperture)) {
        printf("failed at get_spad_info (addr 0x%02X)\n", addr);
        return false;
    }
    printf("spad_info OK: count=%d aperture=%d (addr 0x%02X)\n",
           spad_count, spad_is_aperture, addr);

    vl53l0x_set_spads_and_tuning(addr, spad_count, spad_is_aperture);
    printf("set_spads_and_tuning OK (addr 0x%02X)\n", addr);

    vl53l0x_write_reg(addr, 0x0A, 0x04);                          
    uint8_t gpio_hv = vl53l0x_read_reg(addr, 0x84);               
    vl53l0x_write_reg(addr, 0x84, gpio_hv & ~0x10);               
    vl53l0x_write_reg(addr, INTERRUPT_CLEAR, 0x01);               
    vl53l0x_write_reg(addr, SEQUENCE_CONFIG, 0xE8);

    vl53l0x_write_reg(addr, SEQUENCE_CONFIG, 0x01);
    if (!vl53l0x_perform_single_ref_calibration(addr, 0x40)) {
        printf("failed at VHV calibration (addr 0x%02X)\n", addr);
        return false;
    }
    printf("VHV calibration OK (addr 0x%02X)\n", addr);

    vl53l0x_write_reg(addr, SEQUENCE_CONFIG, 0x02);
    if (!vl53l0x_perform_single_ref_calibration(addr, 0x00)) {
        printf("failed at phase calibration (addr 0x%02X)\n", addr);
        return false;
    }
    printf("phase calibration OK (addr 0x%02X)\n", addr);

    vl53l0x_write_reg(addr, SEQUENCE_CONFIG, 0xE8);
    vl53l0x_start_ranging(sensor);

    return true;
}

bool vl53l0x_init_sensors(vl53l0x_sensor_t *sensors, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        gpio_init(sensors[i].xshut_pin);
        gpio_set_dir(sensors[i].xshut_pin, GPIO_OUT);
        gpio_put(sensors[i].xshut_pin, 0);
    }
    sleep_ms(10);
    for (uint8_t i = 0; i < count; i++)
    {
        if (!vl53l0x_init_sensor(&sensors[i])) {
            return false;
        }
    }
    return true;
}
