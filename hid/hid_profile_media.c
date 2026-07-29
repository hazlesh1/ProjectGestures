#include "hid/hid_profile_media.h"
#include "gesture/gesture_logic.h"

#include "class/hid/hid_device.h"
#include "tusb.h"

#define CONSUMER_PLAY_PAUSE    0x00CD
#define CONSUMER_SCAN_NEXT     0x00B5
#define CONSUMER_SCAN_PREV     0x00B6
#define CONSUMER_VOLUME_UP     0x00E9
#define CONSUMER_VOLUME_DOWN   0x00EA

const uint8_t media_report_desc[] = {
    TUD_HID_REPORT_DESC_CONSUMER()
};

const uint8_t * const hid_report_descriptor = media_report_desc;

static bool media_send_gesture(gesture_event_t ev) {
    uint16_t usage = 0;
    switch (ev) {
        case GESTURE_SWIPE_RIGHT: usage = CONSUMER_VOLUME_UP;     break;
        case GESTURE_SWIPE_LEFT:  usage = CONSUMER_VOLUME_DOWN;   break;
        case GESTURE_SWIPE_UP:    usage = CONSUMER_SCAN_NEXT;     break;
        case GESTURE_SWIPE_DOWN:  usage = CONSUMER_SCAN_PREV;     break;
        case GESTURE_HOVER:       usage = CONSUMER_PLAY_PAUSE;    break;
        default: return false;
    }

    if (!tud_hid_ready()) return false;
    return tud_hid_report(0, &usage, sizeof(usage));
}

static bool media_send_empty(void) {
    if (!tud_hid_ready()) return false;
    uint16_t usage = 0;
    return tud_hid_report(0, &usage, sizeof(usage));
}

const hid_profile_t hid_profile_media = {
    .log_name     = "media (consumer control)",
    .send_gesture = media_send_gesture,
    .send_empty   = media_send_empty,
};