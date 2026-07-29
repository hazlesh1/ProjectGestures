#include "hid_output.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "bsp/board.h"
#include "tusb.h"

#if HID_PROFILE == HID_PROFILE_MEDIA
#include "hid/hid_profile_media.h"
#elif HID_PROFILE == HID_PROFILE_PAGING
#include "hid/hid_profile_paging.h"
#endif

#if HID_PROFILE == HID_PROFILE_MEDIA
static const hid_profile_t *active_profile = &hid_profile_media;
#elif HID_PROFILE == HID_PROFILE_PAGING
static const hid_profile_t *active_profile = &hid_profile_paging;
#else
#error "HID_PROFILE must be HID_PROFILE_MEDIA (1) or HID_PROFILE_PAGING (2)"
#endif

#define HID_RELEASE_RETRY_TIMEOUT_MS 50
#define HID_PRESS_RELEASE_GAP_US     1000

static bool busy = false;

void hid_output_init(void) {
    board_init();
    tud_init(BOARD_TUD_RHPORT);

    if (active_profile && active_profile->log_name) {
        printf("[HID] active profile: %s\n", active_profile->log_name);
    }
}

void hid_output_task(void) {
    tud_task();
}

void hid_output_dispatch(gesture_event_t ev) {
    if (ev == GESTURE_NONE) return;
    if (busy) {
        printf("[HID] dispatch REJECTED: busy flag still set\n");
        return;
    }

    if (!active_profile || !active_profile->send_gesture) return;

    busy = true;

    bool press_ready_before = tud_hid_ready();
    bool press_ok = active_profile->send_gesture(ev);
    printf("[HID] press: ready_before=%d result=%d\n", press_ready_before, press_ok);

    if (!press_ok) {
        busy = false;
        return;
    }

    sleep_us(HID_PRESS_RELEASE_GAP_US);

    if (active_profile->send_empty) {
        absolute_time_t deadline = make_timeout_time_ms(HID_RELEASE_RETRY_TIMEOUT_MS);
        bool sent = false;
        int attempts = 0;
        while (!time_reached(deadline)) {
            attempts++;
            bool ready_now = tud_hid_ready();
            sent = active_profile->send_empty();
            if (attempts <= 5 || sent) {
                printf("[HID] release attempt %d: ready=%d sent=%d\n", attempts, ready_now, sent);
            }
            if (sent) break;
            tud_task();
        }
        if (!sent) {
            printf("[HID] WARNING: release failed after %d attempts within %dms\n",
                   attempts, HID_RELEASE_RETRY_TIMEOUT_MS);
        } else {
            printf("[HID] release OK after %d attempt(s)\n", attempts);
        }
    }

    busy = false;
}

void hid_output_set_led(bool on) {
    (void)on;
}