# ProjectGest — Air Gesture USB HID Controller

This project is a mid-air gesture controller built on a Raspberry Pi Pico (RP2040) and 4 VL53L0X time-of-flight sensors measure distances to detect hand movements with algorithms over an I2C bus. It turns raw distance data into swipe/hover gestures, and the Pico presents itself to the host OS as a native USB HID device so there are no drivers or apps needed to make the gestures work.

---

## Overview

This project is an air gesture media/navigation controller. An array of four IR tof sensors (VL53L0X) surrounds a sensing zone. When a hand swipes through the zone in a given direction (one sensor into another in a specific direction), or hovers in the center (all four sensors), the firmware recognizes the gesture and sends a USB HID report; appearing to the host as a real keyboard or consumer device, with no external drivers or background software required.

## Features

- 4-directional gesture direction: left, right, up, down, and a center hover gesture.
- There are also two selectable HID output profiles (chosen on build):
  - **Media Profile** - Consumer Control (volume up/down, track next/back, play/pause)
  - **Paging Profile** - Keyboard emulation (arrow keys, page up/down, space)
- Zero heap allocations - sensor history and state uses static memory
- Real time visual debug system with 3 LEDs
- Fully native OS enumeration as a HID-class device
- Axis-locking gesture state machine with entry/exit timing windows and depth-symmetry checks

## Hardware Requirements and Wiring

**Apparatus:**
- 1x Raspberry Pi Pico (RP2040)
- 4x VL53L0X time-of-flight sensors
- Breadboard and Jumper wires
- USB cable (data-capable)
- Headers + Solder
- Resistors
- Heat Shrinking tubes (optional)

**Pin Mapping:**

| Signal | GPIO | Purpose |
|---|---|---|
| I2C SDA | GP4 | Shared I2C0 bus, all 4 sensors |
| I2C SCL | GP5 | Shared I2C0 bus, 400 kHz (Fast Mode) |
| XSHUT — Left | GP6 | assigned address 0x30 |
| XSHUT — Right | GP7 | Assigned address 0x31 |
| XSHUT — Up | GP8 | Assigned address 0x32 |
| XSHUT — Down | GP9 | Assigned address 0x33 |
| LED — Cycle Start | GP16 | 10 ms pulse at the start of every poll cycle |
| LED — Gesture Fired | GP17 | 150 ms pulse when gesture |
| LED — Cycle Active | GP18 | High for the full duration of a poll cycle |

The four sensors share a single I2C bus. All VL53L0X boots at the same default address (0x29), each sensor's XSHUT pin is held low at boot and released to reassign address before the next sensor comes online.

## Project Structure

```
.
├── .vscode/
├── drivers/
│   ├── vl53l0x_driver.c
│   └── vl53l0x_driver.h
├── gesture/
│   ├── gesture_logic.c
│   ├── gesture_logic.h
│   ├── history.c
│   └── history.h
├── hid/
│   ├── hid_output.c
│   ├── hid_output.h
│   ├── hid_profile_media.c
│   ├── hid_profile_media.h
│   ├── hid_profile_paging.c
│   └── hid_profile_paging.h
├── .clangd
├── .gitignore
├── CMakeLists.txt
├── main.cpp
├── pico_sdk_import.cmake
├── tusb_config.h
├── usb_cdc_stdio.c
├── usb_cdc_stdio.h
└── usb_descriptors.c
```

## Data Flow

```
[4× VL53L0X sensors] ──► vl53l0x_driver.c ──► history.c ──► gesture_logic.c ──► hid_output.c ──► (hid_profile_media.c / hid_profile_paging.c) ──► usb_descriptors.c + TinyUSB ──► Host OS
```

`main.cpp` ties everything together into a single poll loop.

## Building and Flashing

**Prerequisites:** Pico SDK 2.3.0, ARM GNU Toolchain (15_2_Rel1), CMake ≥ 3.13.

```bash
mkdir build && cd build
cmake -DPICO_BOARD=pico -DHID_PROFILE=1 ..   # 1 = media, 2 = paging
make -j$(nproc)
```

Produces `.uf2`, drag the `.uf2` into the Pico in fastboot mode (holding BOOTSEL when plugging in).

To switch HID profiles, rebuild with `-DHID_PROFILE=2` (default to 1)

## Configuration Reference

| Constant | Value | File | Controls |
|---|---|---|---|
| `SENSOR_POLL_INTERVAL_MS` | 25 | `main.cpp` | rate of the 4 sensors reading |
| `GESTURE_ZONE_MIN_MM` + `MAX` | 30 / 350 | `gesture_logic.h` | valid sensing range |
| `SMOOTHING_SAMPLES` | 2 | `gesture_logic.c` | number of samples averaged |
| `MAX_SWIPE_WINDOW_MS` | 400 | `gesture_logic.c` | in between time max for gesture |
| `GESTURE_COOLDOWN_MS` | 500 | `gesture_logic.c` | Min gap between two gestures |
| `MIN_OVERLAP_MS` | 60 | `gesture_logic.c` | Min time both sensors in-zone |
| `DEPTH_MATCH_TOLERANCE` | 200 | `gesture_logic.c` | Max allowed entry/exit depth mismatch |
| `HOVER_DURATION_MS` | 1500 | `gesture_logic.c` | How long all 4 sensors must stay in-zone to fire hover gesture |
| `HOVER_VARIANCE_MAX_MM` | 150 | `gesture_logic.c` | Max spread between sensors during hover |
| `AXIS_TIMEOUT_MS` | 600 | `gesture_logic.c` | Time axis lock is held before release (ver/hor) |
| `HISTORY_SIZE` | 10 | `history.h` | samples retained (available for use) |

## Gesture Detection Logic

The core challenge here is to distinguish left to right, vice versa and up down swipes from noisy data, without letting a horizontal gesture accidentally trigger a vertical one and vice versa (as our arms and hands always pass through at least 3 sensors in one gesture.)

An axis-lock state machine is used. When either horizontal or vertical pair sees a sensor enter its zone, the axis is locked for (`AXIS_TIMEOUT_MS`); making the opposite axis sensor ignored until the current gesture is fired or times out.

```c
if (axis_lock == AXIS_UNLOCKED
 && (nowMs - last_gesture_ms) > GESTURE_COOLDOWN_MS) {
    if (rL || rR) {
        axis_lock = AXIS_LOCKED_HORIZONTAL;
        axis_lock_acquired_ms = nowMs;
        cycle_consumed = false;
    } else if (rU || rD) {
        axis_lock = AXIS_LOCKED_VERTICAL;
        axis_lock_acquired_ms = nowMs;
        cycle_consumed = false;
    }
}
```

`eval_axis()` waits for both sensors on that axis to have entered and exited the zone.

The function checks:

- Depth Symmetry (`DEPTH_MATCH_TOLERANCE`)
- Timing window (`MAX_SWIPE_WINDOW_MS`)
- Overlap *(Not on by default)*, (`MIN_OVERLAP_MS`)

```c
if (!e_valid && !x_valid) {
    printf("[DEBUG] AXIS REJECTED: both entryDelta and exitDelta out of window (%u ms)\n",
           MAX_SWIPE_WINDOW_MS);
} else if (e_valid) {
    r = (a->entered_ms < b->entered_ms) ? swipe_a_first : swipe_b_first;
    if (had_overlap) {
        printf("[DEBUG] AXIS FIRED (by entry order): %d\n", (int)r);
    } else {
        printf("[DEBUG] AXIS FIRED (by entry order, no overlap): %d\n", (int)r);
    }
} else {
    r = (a->exited_ms  < b->exited_ms)  ? swipe_b_first : swipe_a_first;
    if (had_overlap) {
        printf("[DEBUG] AXIS FIRED (by exit order): %d\n", (int)r);
    } else {
        printf("[DEBUG] AXIS FIRED (by exit order, no overlap): %d\n", (int)r);
    }
}
```

If all 3 hold, direction is derived based on the sensor that received an entry first.

If all 4 hold, another function evaluates it separately based on (`HOVER_DURATION_MS`) and (`HOVER_VARIANCE_MAX_MM`):

```c
if (hoverL && hoverR && hoverU && hoverD) {
    uint16_t mn = aL, mx = aL;
    if ((mx - mn) <= HOVER_VARIANCE_MAX_MM) {
        if (!hover_active) { hover_start_ms = nowMs; hover_active = true; }
        if (!hover_fired && (nowMs - hover_start_ms) >= HOVER_DURATION_MS) {
            hover_fired = true;
            return GESTURE_HOVER;
        }
    }
}
```

## VL53L0X I2C Driver

Each sensor is hardwired to the same I2C address by default, which is 0x29. When using four of them onto one bus, it requires a bring_up sequence: hold every sensor's XSHUT pin low at boot (disabling it), then bring them online one at a time, assigning each to a unique address.

```c
bool vl53l0x_init_sensors(vl53l0x_sensor_t *sensors, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++) {
        gpio_init(sensors[i].xshut_pin);
        gpio_set_dir(sensors[i].xshut_pin, GPIO_OUT);
        gpio_put(sensors[i].xshut_pin, 0);   // hold all sensors in reset
    }
    sleep_ms(10);
    for (uint8_t i = 0; i < count; i++) {
        if (!vl53l0x_init_sensor(&sensors[i])) return false;
    }
    return true;
}
```

In the init function, each sensor is brought out of reset given a new address via `I2C_ADDR_CHANGE_REG` (0x8A), then walked through the sensor's factory calibration sequence (from Pololu VL53L0X Arduino C++ library)

Function running on each sensor at initialization:

```c
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
```

This includes SPAD, tuning, VHV calibration and continuous ranging start.

## USB HID Layer

This project builds one of two HID report descriptors depending on the `HID_PROFILE` CMake flag:

```c
const uint8_t paging_report_desc[] = { TUD_HID_REPORT_DESC_KEYBOARD() }; 
const uint8_t media_report_desc[] = { TUD_HID_REPORT_DESC_CONSUMER() }; 
```

If not defined:

```cmake
if(NOT DEFINED HID_PROFILE)
    set(HID_PROFILE 1)
endif()
```

This implements a small interface so `hid_output.c` does not need to know which is active. It calls `send_gesture()` and `send_empty()` through a function-pointer struct:

```c
typedef struct {
    const char *log_name;
    bool (*send_gesture)(gesture_event_t ev);
    bool (*send_empty)(void);
} hid_profile_t;
```

**Media Profile:**

| Gesture | Consumer Control | Usage |
|---|---|---|
| Swipe Right | Volume Up | 0x00E9 |
| Swipe Left | Volume Down | 0x00EA |
| Swipe Up | Scan Next Track | 0x00B5 |
| Swipe Down | Scan Previous Track | 0x00B6 |
| Hover | Play/Pause | 0x00CD |

**Paging Profile:**

| Gesture | Key | Keyboard Usage Code |
|---|---|---|
| Swipe Right | Right Arrow | 0x004F |
| Swipe Left | Left Arrow | 0x0050 |
| Swipe Up | Page Up | 0x004B |
| Swipe Down | Page Down | 0x004E |
| Hover | Spacebar | 0x002C |

## Debugging Tools

| LED | Pin | Meaning |
|---|---|---|
| Cycle Active | GP18 | High for the full duration of a poll cycle |
| Cycle Start | GP16 | Pulses 10 ms at the start of every poll |
| Gesture Fired | GP17 | Pulses 150 ms whenever a gesture is dispatched |

This project also routes `printf()` over a USB-CDC virtual serial port (`usb_cdc_stdio.c`), debug output is available in serial monitors.

## Known Limitations / Troubleshooting

- **Development VID/PID:** `usb_descriptors.c` uses shared open-source test VID/PID pair (0x16C0/0x05DB) commonly used by hobbyist USB projects.
- **Low smoothing window:** `SMOOTHING_SAMPLES` is set to 2 by default. This trades noise rejection for lower latency, adjust depending on use case.
- **Axis-lock timeout:** a locked axis auto-releases after `AXIS_TIMEOUT_MS` (600 ms) if no valid gesture completes, slow or hesitant gestures may get cut off before completing, adjust depending on use case.

## Acknowledgments

- **Pololu VL53L0X Arduino Library:** reference for register sequencing and calibration steps in `vl53l0x_driver.c`
- **ST VL53L0X datasheet / register map:** source for calibration register values
- **Raspberry Pi Pico SDK and TinyUSB:** hardware abstraction and USB stack

## License

See [LICENSE](./LICENSE) for details.