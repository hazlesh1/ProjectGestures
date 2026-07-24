#include "hid_output.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "bsp/board.h"
#include "tusb.h"

#include "hid/hid_profile_media.h"
#include "hid/hid_profile_paging.h"

#if HID_PROFILE == HID_PROFILE_MEDIA
static const hid_profile_t *active_profile = &hid_profile_media;
#elif HID_PROFILE == HID_PROFILE_PAGING
static const hid_profile_t *active_profile = &hid_profile_paging;
#else
#error "HID_PROFILE must be HID_PROFILE_MEDIA (1) or HID_PROFILE_PAGING (2)"
#endif

typedef enum {
    HID_FSM_IDLE = 0,
    HID_FSM_HOLD,
    HID_FSM_RELEASING,
} hid_fsm_state_t;

static hid_fsm_state_t fsm_state          = HID_FSM_IDLE;
static uint32_t         fsm_deadline_ms    = 0;
static uint32_t         release_timeout_ms = 0;

void hid_output_init(void) {
    board_init();
    tud_init(BOARD_TUD_RHPORT);

    if (active_profile && active_profile->log_name) {
        printf("[HID] active profile: %s\n", active_profile->log_name);
    }
}

void hid_output_task(void) {
    tud_task();

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    if (fsm_state == HID_FSM_HOLD && time_reached(fsm_deadline_ms)) {
        fsm_state          = HID_FSM_RELEASING;
        release_timeout_ms = now_ms + HID_RELEASE_RETRY_TIMEOUT_MS;
    }

    if (fsm_state == HID_FSM_RELEASING) {
        bool sent = false;
        if (active_profile && active_profile->send_empty) {
            sent = active_profile->send_empty();
        }

        if (sent) {
            fsm_state = HID_FSM_IDLE;
        } else if (time_reached(release_timeout_ms)) {
            printf("[HID] WARNING: release report failed to send within %dms\n",
                   HID_RELEASE_RETRY_TIMEOUT_MS);
            fsm_state = HID_FSM_IDLE;
        }
    }
}

void hid_output_dispatch(gesture_event_t ev) {
    if (ev == GESTURE_NONE) return;
    if (fsm_state != HID_FSM_IDLE) return;

    if (!active_profile || !active_profile->send_gesture) return;
    if (!active_profile->send_gesture(ev)) return;

    fsm_state       = HID_FSM_HOLD;
    fsm_deadline_ms = to_ms_since_boot(get_absolute_time()) + HID_KEY_HOLD_MS;
}

void hid_output_set_led(bool on) {
    (void)on;
}