// gesture_logic.c
#include "gesture_logic.h"
#include "sensor_history.h"
#include "vl53l0x_driver.h" // for VL53L0X_NOT_READY

static sensor_history_t left_history;
static sensor_history_t right_history;

void gesture_logic_init(void) {
    history_init(&left_history);
    history_init(&right_history);
}

static bool in_zone(uint16_t mm) {
    if (mm == VL53L0X_NOT_READY) return false;
    return (mm >= GESTURE_ZONE_MIN_MM) && (mm <= GESTURE_ZONE_MAX_MM);
}

gesture_event_t gesture_logic_update(uint16_t left_mm, uint16_t right_mm, uint32_t timestamp_ms) {
    history_push(&left_history, left_mm, timestamp_ms);
    history_push(&right_history, right_mm, timestamp_ms);

    // TODO: detection algorithm
    //
    //   - in_zone(left_mm), in_zone(right_mm)  -> is the hand in range right now
    //   - history_get(&left_history, n, &sample)  -> look back n samples
    //   - sample.distance_mm, sample.timestamp_ms

    //   1. Find the most recent timestamp where left go in
    //   2. Find the most recent timestamp where right go in
    //   3. If left entry time is earlier than right (idk
    //      max gesture duration, e.g. 300ms), left->right swipe.
    //   4. If right entered first, right->left swipe.
    //   5. If both in-zone and stay that way for a while without one
    //      leading the other, hover.

    return GESTURE_NONE;
}