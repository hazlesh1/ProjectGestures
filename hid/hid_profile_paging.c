#include "hid/hid_profile_paging.h"
#include "gesture/gesture_logic.h"

#include "class/hid/hid_device.h"
#include "tusb.h"

#define HID_KEY_NONE         0x00
#define HID_KEY_SPACE        0x2C
#define HID_KEY_PAGE_UP      0x4B
#define HID_KEY_PAGE_DOWN    0x4E
#define HID_KEY_RIGHT_ARROW  0x4F
#define HID_KEY_LEFT_ARROW   0x50
#define HID_KEY_DOWN_ARROW   0x51
#define HID_KEY_UP_ARROW     0x52

#define HID_MOD_NONE  0x00

const uint8_t paging_report_desc[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

const uint8_t * const hid_report_descriptor = paging_report_desc;

typedef struct __attribute__((packed)) {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keys[6];
} paging_report_t;

static bool paging_send_gesture(gesture_event_t ev) {
    uint8_t key = HID_KEY_NONE;
    switch (ev) {
        case GESTURE_SWIPE_RIGHT: key = HID_KEY_RIGHT_ARROW; break;
        case GESTURE_SWIPE_LEFT:  key = HID_KEY_LEFT_ARROW;  break;
        case GESTURE_SWIPE_UP:    key = HID_KEY_PAGE_UP;     break;
        case GESTURE_SWIPE_DOWN:  key = HID_KEY_PAGE_DOWN;   break;
        case GESTURE_HOVER:       key = HID_KEY_SPACE;       break;
        default: return false;
    }

    if (!tud_hid_ready()) return false;
    paging_report_t r = {
        .modifier = HID_MOD_NONE,
        .reserved = 0,
        .keys     = { key, 0, 0, 0, 0, 0 },
    };
    return tud_hid_report(0, &r, sizeof(r));
}

static bool paging_send_empty(void) {
    if (!tud_hid_ready()) return false;
    paging_report_t r = { 0 };
    return tud_hid_report(0, &r, sizeof(r));
}

const hid_profile_t hid_profile_paging = {
    .log_name     = "paging (boot keyboard)",
    .send_gesture = paging_send_gesture,
    .send_empty   = paging_send_empty,
};