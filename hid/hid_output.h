#ifndef HID_OUTPUT_H
#define HID_OUTPUT_H

#include <stdint.h>
#include <stdbool.h>
#include "gesture/gesture_logic.h"

#define HID_PROFILE_MEDIA   1
#define HID_PROFILE_PAGING  2

#define HID_KEY_HOLD_MS 10
#define HID_RELEASE_RETRY_TIMEOUT_MS 200

typedef struct {
    const char *log_name;
    bool (*send_gesture)(gesture_event_t ev);
    bool (*send_empty)(void);
} hid_profile_t;

extern const uint8_t * const hid_report_descriptor;

#ifdef __cplusplus
extern "C" {
#endif

void hid_output_init(void);
void hid_output_task(void);
void hid_output_dispatch(gesture_event_t ev);
void hid_output_set_led(bool on);

#ifdef __cplusplus
}
#endif

#endif