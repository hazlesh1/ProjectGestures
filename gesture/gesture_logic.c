// #include "gesture_logic.h"
// #include "history.h"
// #include "vl53l0x_driver.h"
// #include <stdio.h>

// #define SMOOTHING_SAMPLES 5
// #define MAX_SWIPE_WINDOW_MS 400
// #define GESTURE_COOLDOWN_MS 500
// #define MIN_OVERLAP_MS 60

// static sensor_history_t left_history;
// static sensor_history_t right_history;

// static uint32_t left_entered_ms = 0;
// static uint32_t left_exited_ms = 0;
// static uint32_t right_entered_ms = 0;
// static uint32_t right_exited_ms = 0;

// static bool left_was_in_zone = false;
// static bool right_was_in_zone = false;

// static bool cycle_consumed = true;
// static uint32_t last_gesture_ms = 0;

// void gesture_logic_init(void) {
//     history_init(&left_history);
//     history_init(&right_history);
//     left_was_in_zone = false;
//     right_was_in_zone = false;
//     cycle_consumed = true;
//     last_gesture_ms = 0;
// }

// static bool inZone(uint16_t mm) {
//     if (mm == VL53L0X_NOT_READY) return false;
//     return (mm >= GESTURE_ZONE_MIN_MM) && (mm <= GESTURE_ZONE_MAX_MM);
// }

// gesture_event_t gesture_logic_update(uint16_t leftMm, uint16_t rightMm, uint32_t nowMs) {
//     history_push(&left_history, leftMm, nowMs);
//     history_push(&right_history, rightMm, nowMs);

//     uint16_t leftAvg  = history_average(&left_history, SMOOTHING_SAMPLES);
//     uint16_t rightAvg = history_average(&right_history, SMOOTHING_SAMPLES);

//     bool leftInZone  = inZone(leftAvg);
//     bool rightInZone = inZone(rightAvg);

//     if (leftInZone && !left_was_in_zone) {
//         left_entered_ms = nowMs;
//         cycle_consumed = false;
//         printf("[DEBUG] LEFT entered zone at %u\n", nowMs);
//     }
//     if (rightInZone && !right_was_in_zone) {
//         right_entered_ms = nowMs;
//         cycle_consumed = false;
//         printf("[DEBUG] RIGHT entered zone at %u\n", nowMs);
//     }

//     if (!leftInZone && left_was_in_zone) {
//         left_exited_ms = nowMs;
//         printf("[DEBUG] LEFT exited zone at %u\n", nowMs);
//     }
//     if (!rightInZone && right_was_in_zone) {
//         right_exited_ms = nowMs;
//         printf("[DEBUG] RIGHT exited zone at %u\n", nowMs);
//     }

//     left_was_in_zone  = leftInZone;
//     right_was_in_zone = rightInZone;

//     gesture_event_t result = GESTURE_NONE;
//     bool cooldownOk = (nowMs - last_gesture_ms) > GESTURE_COOLDOWN_MS;

//     bool bothExited = (left_exited_ms > left_entered_ms) &&
//                        (right_exited_ms > right_entered_ms);

//     if (bothExited && !cycle_consumed) {
//         uint32_t entryDiff = (left_entered_ms > right_entered_ms)
//             ? (left_entered_ms - right_entered_ms)
//             : (right_entered_ms - left_entered_ms);
//         uint32_t exitDiff = (left_exited_ms > right_exited_ms)
//             ? (left_exited_ms - right_exited_ms)
//             : (right_exited_ms - left_exited_ms);

//         bool entryOrderValid = entryDiff <= MAX_SWIPE_WINDOW_MS && entryDiff > 0;
//         bool exitOrderValid  = exitDiff  <= MAX_SWIPE_WINDOW_MS && exitDiff  > 0;

//         uint32_t overlapStart = (left_entered_ms > right_entered_ms) ? left_entered_ms : right_entered_ms;
//         uint32_t overlapEnd   = (left_exited_ms  < right_exited_ms)  ? left_exited_ms  : right_exited_ms;
//         bool hadRealOverlap = (overlapEnd > overlapStart) && ((overlapEnd - overlapStart) >= MIN_OVERLAP_MS);

//         bool entrySaysLeftFirst = left_entered_ms < right_entered_ms;
//         bool exitSaysLeftFirst  = left_exited_ms  < right_exited_ms;
//         bool ordersAgree = (entrySaysLeftFirst == exitSaysLeftFirst);

//         printf("[DEBUG] CYCLE COMPLETE: L_enter=%u L_exit=%u R_enter=%u R_exit=%u\n",
//                left_entered_ms, left_exited_ms, right_entered_ms, right_exited_ms);
//         printf("[DEBUG]   entryDiff=%u exitDiff=%u overlapStart=%u overlapEnd=%u overlapLen=%d\n",
//                entryDiff, exitDiff, overlapStart, overlapEnd, (int)overlapEnd - (int)overlapStart);
//         printf("[DEBUG]   entryOrderValid=%d exitOrderValid=%d hadRealOverlap=%d ordersAgree=%d cooldownOk=%d\n",
//                entryOrderValid, exitOrderValid, hadRealOverlap, ordersAgree, cooldownOk);

//         if (cooldownOk) {
//             if (hadRealOverlap && ordersAgree && exitOrderValid && entryOrderValid) {
//                 result = (left_exited_ms < right_exited_ms) ? GESTURE_SWIPE_LEFT : GESTURE_SWIPE_RIGHT;
//             } else if (hadRealOverlap && entryOrderValid && !exitOrderValid) {
//                 result = (left_entered_ms < right_entered_ms) ? GESTURE_SWIPE_RIGHT : GESTURE_SWIPE_LEFT;
//             }
//         }

//         if (result != GESTURE_NONE) {
//             cycle_consumed = true;
//             printf("[DEBUG]   -> FIRED: %d\n", (int)result);
//         } else {
//             cycle_consumed = true; 
//             printf("[DEBUG]   -> REJECTED\n");
//         }
//     }

//     if (result != GESTURE_NONE) {
//         last_gesture_ms = nowMs;
//     }

//     return result;
// }

// ============================================================================
// ACTIVE 2-SENSOR (LEFT/RIGHT) CODE — REPLACED BY 4-SENSOR REWRITE BELOW
// Per rules: "If you change code. do not remove it. Comment it out."
// ============================================================================
/*
#include "gesture_logic.h"
#include "history.h"
#include "vl53l0x_driver.h"
#include <stdio.h>

#define SMOOTHING_SAMPLES 5
#define MAX_SWIPE_WINDOW_MS 400
#define GESTURE_COOLDOWN_MS 500
#define MIN_OVERLAP_MS 60

static sensor_history_t left_history;
static sensor_history_t right_history;

static uint32_t left_entered_ms = 0;
static uint32_t left_exited_ms = 0;
static uint32_t right_entered_ms = 0;
static uint32_t right_exited_ms = 0;

static bool left_was_in_zone = false;
static bool right_was_in_zone = false;

static bool cycle_consumed = true;
static uint32_t last_gesture_ms = 0;

void gesture_logic_init(void) {
    history_init(&left_history);
    history_init(&right_history);
    left_was_in_zone = false;
    right_was_in_zone = false;
    cycle_consumed = true;
    last_gesture_ms = 0;
}

static bool inZone(uint16_t mm) {
    if (mm == VL53L0X_NOT_READY) return false;
    return (mm >= GESTURE_ZONE_MIN_MM) && (mm <= GESTURE_ZONE_MAX_MM);
}

gesture_event_t gesture_logic_update(uint16_t leftMm, uint16_t rightMm, uint32_t nowMs) {
    history_push(&left_history, leftMm, nowMs);
    history_push(&right_history, rightMm, nowMs);

    uint16_t leftAvg  = history_average(&left_history, SMOOTHING_SAMPLES);
    uint16_t rightAvg = history_average(&right_history, SMOOTHING_SAMPLES);

    bool leftInZone  = inZone(leftAvg);
    bool rightInZone = inZone(rightAvg);

    if (leftInZone && !left_was_in_zone) {
        left_entered_ms = nowMs;
        cycle_consumed = false;
        printf("[DEBUG] LEFT entered zone at %u\n", nowMs);
    }
    if (rightInZone && !right_was_in_zone) {
        right_entered_ms = nowMs;
        cycle_consumed = false;
        printf("[DEBUG] RIGHT entered zone at %u\n", nowMs);
    }

    if (!leftInZone && left_was_in_zone) {
        left_exited_ms = nowMs;
        printf("[DEBUG] LEFT exited zone at %u\n", nowMs);
    }
    if (!rightInZone && right_was_in_zone) {
        right_exited_ms = nowMs;
        printf("[DEBUG] RIGHT exited zone at %u\n", nowMs);
    }

    left_was_in_zone  = leftInZone;
    right_was_in_zone = rightInZone;

    gesture_event_t result = GESTURE_NONE;
    bool cooldownOk = (nowMs - last_gesture_ms) > GESTURE_COOLDOWN_MS;

    bool bothExited = (left_exited_ms > left_entered_ms) &&
                       (right_exited_ms > right_entered_ms);

    if (bothExited && !cycle_consumed) {
        uint32_t entryDiff = (left_entered_ms > right_entered_ms)
            ? (left_entered_ms - right_entered_ms)
            : (right_entered_ms - left_entered_ms);
        uint32_t exitDiff = (left_exited_ms > right_exited_ms)
            ? (left_exited_ms - right_exited_ms)
            : (right_exited_ms - left_exited_ms);

        bool entryOrderValid = entryDiff <= MAX_SWIPE_WINDOW_MS && entryDiff > 0;
        bool exitOrderValid  = exitDiff  <= MAX_SWIPE_WINDOW_MS && exitDiff  > 0;

        uint32_t overlapStart = (left_entered_ms > right_entered_ms) ? left_entered_ms : right_entered_ms;
        uint32_t overlapEnd   = (left_exited_ms  < right_exited_ms)  ? left_exited_ms  : right_exited_ms;
        bool hadRealOverlap = (overlapEnd > overlapStart) && ((overlapEnd - overlapStart) >= MIN_OVERLAP_MS);

        printf("[DEBUG] CYCLE COMPLETE: L_enter=%u L_exit=%u R_enter=%u R_exit=%u\n",
               left_entered_ms, left_exited_ms, right_entered_ms, right_exited_ms);
        printf("[DEBUG]   entryDiff=%u exitDiff=%u overlapStart=%u overlapEnd=%u overlapLen=%d\n",
               entryDiff, exitDiff, overlapStart, overlapEnd, (int)overlapEnd - (int)overlapStart);
        printf("[DEBUG]   entryOrderValid=%d exitOrderValid=%d hadRealOverlap=%d cooldownOk=%d\n",
               entryOrderValid, exitOrderValid, hadRealOverlap, cooldownOk);

        if (cooldownOk && hadRealOverlap) {
            if (entryOrderValid) {
                result = (left_entered_ms < right_entered_ms) ? GESTURE_SWIPE_RIGHT : GESTURE_SWIPE_LEFT;
            } else if (exitOrderValid) {
                result = (left_exited_ms < right_exited_ms) ? GESTURE_SWIPE_LEFT : GESTURE_SWIPE_RIGHT;
            }
        }

        cycle_consumed = true;

        if (result != GESTURE_NONE) {
            printf("[DEBUG]   -> FIRED: %d\n", (int)result);
        } else {
            printf("[DEBUG]   -> REJECTED\n");
        }
    }

    if (result != GESTURE_NONE) {
        last_gesture_ms = nowMs;
    }

    return result;
}
*/

#include "gesture_logic.h"
#include "history.h"
#include "vl53l0x_driver.h"
#include <stdio.h>
#include <stdlib.h>

#define SMOOTHING_SAMPLES         2
#define MAX_SWIPE_WINDOW_MS       400
#define GESTURE_COOLDOWN_MS       500
#define MIN_OVERLAP_MS            60
#define DEPTH_MATCH_TOLERANCE     200
#define HOVER_DURATION_MS         1500 
#define HOVER_VARIANCE_MAX_MM     40    
#define AXIS_TIMEOUT_MS           600   

typedef enum {
    AXIS_UNLOCKED = 0,
    AXIS_LOCKED_HORIZONTAL,
    AXIS_LOCKED_VERTICAL,
} axis_lock_t;

typedef struct {
    uint32_t entered_ms;
    uint32_t exited_ms;
    uint16_t entry_depth_mm;
    uint16_t exit_depth_mm;
    bool was_in_zone;
} sensor_state_t;

static sensor_history_t hist_l, hist_r, hist_u, hist_d;
static sensor_state_t  st_l, st_r, st_u, st_d;

static axis_lock_t axis_lock = AXIS_UNLOCKED;
static uint32_t    axis_lock_acquired_ms = 0;
static bool        cycle_consumed = true;
static uint32_t    last_gesture_ms = 0;

static uint32_t hover_start_ms = 0;
static bool     hover_active   = false;
static bool     hover_fired    = false;

void gesture_logic_init(void) {
    history_init(&hist_l);
    history_init(&hist_r);
    history_init(&hist_u);
    history_init(&hist_d);
    st_l = st_r = st_u = st_d = (sensor_state_t){0};
    axis_lock = AXIS_UNLOCKED;
    axis_lock_acquired_ms = 0;
    cycle_consumed = true;
    last_gesture_ms = 0;
    hover_start_ms = 0;
    hover_active = hover_fired = false;
}

static bool inZone(uint16_t mm) {
    return mm != VL53L0X_NOT_READY
        && mm >= GESTURE_ZONE_MIN_MM
        && mm <= GESTURE_ZONE_MAX_MM;
}

static bool update_sensor(sensor_state_t *s, uint16_t avg_mm, uint32_t nowMs) {
    bool in_zone = inZone(avg_mm);
    bool rising  = in_zone && !s->was_in_zone;
    bool falling = !in_zone && s->was_in_zone;
    if (rising) {
        s->entered_ms = nowMs;
        s->entry_depth_mm = avg_mm;
    }
    if (falling) {
        s->exited_ms = nowMs;
        s->exit_depth_mm = avg_mm;
    }
    s->was_in_zone = in_zone;
    return rising;
}


static bool depth_symmetric(uint16_t e, uint16_t x) {
    if (x == VL53L0X_NOT_READY) return true;
    int d = (int)e - (int)x;
    if (d < 0) d = -d;
    return d <= DEPTH_MATCH_TOLERANCE;
}


static gesture_event_t eval_axis(sensor_state_t *a, sensor_state_t *b,
                                 uint32_t nowMs,
                                 gesture_event_t swipe_a_first,
                                 gesture_event_t swipe_b_first) {
    static const char *last_axis_log = NULL;

    if (cycle_consumed) {
        if (last_axis_log != "no cycle") {
            printf("[DEBUG] AXIS REJECTED: no active cycle (cycle_consumed)\n");
            last_axis_log = "no cycle";
        }
        return GESTURE_NONE;
    }
    if (!(a->exited_ms > a->entered_ms)) {
        const char *tag = (a->entered_ms == 0) ? "A not entered yet"
                                                : "A hasn't exited";
        if (last_axis_log != tag) {
            printf("[DEBUG] AXIS REJECTED: sensor %s (enter=%u exit=%u)\n",
                   tag, a->entered_ms, a->exited_ms);
            last_axis_log = tag;
        }
        return GESTURE_NONE;
    }
    if (!(b->exited_ms > b->entered_ms)) {
        const char *tag = (b->entered_ms == 0) ? "B not entered yet"
                                                : "B hasn't exited";
        if (last_axis_log != tag) {
            printf("[DEBUG] AXIS REJECTED: sensor %s (enter=%u exit=%u)\n",
                   tag, b->entered_ms, b->exited_ms);
            last_axis_log = tag;
        }
        return GESTURE_NONE;
    }
    if (a->was_in_zone || b->was_in_zone) {
        if (last_axis_log != "still in zone") {
            printf("[DEBUG] AXIS REJECTED: a sensor still in zone (A=%d B=%d)\n",
                   a->was_in_zone, b->was_in_zone);
            last_axis_log = "still in zone";
        }
        return GESTURE_NONE;
    }

    if ((nowMs - last_gesture_ms) <= GESTURE_COOLDOWN_MS) {
        if (last_axis_log != "cooldown") {
            printf("[DEBUG] AXIS REJECTED: cooldown (delta=%u ms, need >%d)\n",
                   (unsigned)(nowMs - last_gesture_ms), GESTURE_COOLDOWN_MS);
            last_axis_log = "cooldown";
        }
        cycle_consumed = true;
        return GESTURE_NONE;
    }

    if (!depth_symmetric(a->entry_depth_mm, a->exit_depth_mm)) {
        if (last_axis_log != "depth A") {
            printf("[DEBUG] AXIS REJECTED: depth mismatch A (entry=%u exit=%u delta=%d tol=%d)\n",
                   a->entry_depth_mm, a->exit_depth_mm,
                   abs((int)a->entry_depth_mm - (int)a->exit_depth_mm),
                   DEPTH_MATCH_TOLERANCE);
            last_axis_log = "depth A";
        }
        cycle_consumed = true;
        return GESTURE_NONE;
    }
    if (!depth_symmetric(b->entry_depth_mm, b->exit_depth_mm)) {
        if (last_axis_log != "depth B") {
            printf("[DEBUG] AXIS REJECTED: depth mismatch B (entry=%u exit=%u delta=%d tol=%d)\n",
                   b->entry_depth_mm, b->exit_depth_mm,
               abs((int)b->entry_depth_mm - (int)b->exit_depth_mm),
               DEPTH_MATCH_TOLERANCE);
            last_axis_log = "depth B";
        }
        cycle_consumed = true;
        return GESTURE_NONE;
    }

    uint32_t e_d = a->entered_ms > b->entered_ms ? a->entered_ms - b->entered_ms
                                                  : b->entered_ms - a->entered_ms;
    uint32_t x_d = a->exited_ms  > b->exited_ms  ? a->exited_ms  - b->exited_ms
                                                  : b->exited_ms  - a->exited_ms;
    bool e_valid = e_d > 0 && e_d <= MAX_SWIPE_WINDOW_MS;
    bool x_valid = x_d > 0 && x_d <= MAX_SWIPE_WINDOW_MS;

    uint32_t ov_s  = a->entered_ms > b->entered_ms ? a->entered_ms : b->entered_ms;
    uint32_t ov_e  = a->exited_ms  < b->exited_ms  ? a->exited_ms  : b->exited_ms;
    uint32_t overlap_len = (ov_e > ov_s) ? (ov_e - ov_s) : 0;
    bool had_overlap = overlap_len >= MIN_OVERLAP_MS;

    printf("[DEBUG] AXIS CYCLE: A(enter=%u exit=%u depth=%u->%u) "
           "B(enter=%u exit=%u depth=%u->%u) entryDelta=%u exitDelta=%u overlap=%u ms\n",
           a->entered_ms, a->exited_ms, a->entry_depth_mm, a->exit_depth_mm,
           b->entered_ms, b->exited_ms, b->entry_depth_mm, b->exit_depth_mm,
           e_d, x_d, overlap_len);

    gesture_event_t r = GESTURE_NONE;
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

    cycle_consumed = true;
    last_axis_log = NULL;
    return r;
}



gesture_event_t gesture_logic_update(uint16_t leftMm, uint16_t rightMm,
                                     uint16_t upMm, uint16_t downMm,
                                     uint32_t nowMs) {
    history_push(&hist_l, leftMm,  nowMs);
    history_push(&hist_r, rightMm, nowMs);
    history_push(&hist_u, upMm,    nowMs);
    history_push(&hist_d, downMm,  nowMs);

    uint16_t aL = history_average(&hist_l, SMOOTHING_SAMPLES);
    uint16_t aR = history_average(&hist_r, SMOOTHING_SAMPLES);
    uint16_t aU = history_average(&hist_u, SMOOTHING_SAMPLES);
    uint16_t aD = history_average(&hist_d, SMOOTHING_SAMPLES);

    bool horiz_owned = (axis_lock == AXIS_LOCKED_HORIZONTAL);
    bool vert_owned  = (axis_lock == AXIS_LOCKED_VERTICAL);

    bool rL = false, rR = false, rU = false, rD = false;
    if (!vert_owned)  { rL = update_sensor(&st_l, aL, nowMs);
                        rR = update_sensor(&st_r, aR, nowMs); }
    if (!horiz_owned) { rU = update_sensor(&st_u, aU, nowMs);
                        rD = update_sensor(&st_d, aD, nowMs); }

    bool L = horiz_owned ? false : st_l.was_in_zone;
    bool R = horiz_owned ? false : st_r.was_in_zone;
    bool U = vert_owned  ? false : st_u.was_in_zone;
    bool D = vert_owned  ? false : st_d.was_in_zone;

    if (L && R && U && D) {
        uint16_t mn = aL, mx = aL;
        if (aR < mn) mn = aR; if (aR > mx) mx = aR;
        if (aU < mn) mn = aU; if (aU > mx) mx = aU;
        if (aD < mn) mn = aD; if (aD > mx) mx = aD;
        if ((mx - mn) <= HOVER_VARIANCE_MAX_MM) {
            if (!hover_active) { hover_start_ms = nowMs; hover_active = true; }
            if (!hover_fired && (nowMs - hover_start_ms) >= HOVER_DURATION_MS) {
                hover_fired = true;
                last_gesture_ms = nowMs;
                printf("[DEBUG] HOVER FIRED\n");
                return GESTURE_HOVER;
            }
        } else {
            hover_active = false; hover_start_ms = 0;
        }
    } else {
        hover_active = false; hover_start_ms = 0; hover_fired = false;
    }

    if (axis_lock == AXIS_UNLOCKED
     && (nowMs - last_gesture_ms) > GESTURE_COOLDOWN_MS) {
        if (rL || rR) {
            axis_lock = AXIS_LOCKED_HORIZONTAL;
            axis_lock_acquired_ms = nowMs;
            cycle_consumed = false;
            printf("[DEBUG] AXIS LOCK: HORIZONTAL\n");
        } else if (rU || rD) {
            axis_lock = AXIS_LOCKED_VERTICAL;
            axis_lock_acquired_ms = nowMs;
            cycle_consumed = false;
            printf("[DEBUG] AXIS LOCK: VERTICAL\n");
        }
    }

    if (axis_lock != AXIS_UNLOCKED &&
        (nowMs - axis_lock_acquired_ms) > AXIS_TIMEOUT_MS) {
        printf("[DEBUG] AXIS LOCK TIMEOUT, releasing\n");
        cycle_consumed = true;
        axis_lock = AXIS_UNLOCKED;
        axis_lock_acquired_ms = 0;
        if (horiz_owned) { st_l = st_r = (sensor_state_t){0}; }
        if (vert_owned)  { st_u = st_d = (sensor_state_t){0}; }
    }

    gesture_event_t swipe = GESTURE_NONE;
    if (axis_lock == AXIS_LOCKED_HORIZONTAL) {
        swipe = eval_axis(&st_l, &st_r, nowMs, GESTURE_SWIPE_RIGHT, GESTURE_SWIPE_LEFT);
    } else if (axis_lock == AXIS_LOCKED_VERTICAL) {
        swipe = eval_axis(&st_u, &st_d, nowMs, GESTURE_SWIPE_DOWN, GESTURE_SWIPE_UP);
    }

    if (cycle_consumed && axis_lock != AXIS_UNLOCKED) {
        axis_lock = AXIS_UNLOCKED;
        axis_lock_acquired_ms = 0;
        st_l = st_r = st_u = st_d = (sensor_state_t){0};
    }

    if (swipe != GESTURE_NONE) last_gesture_ms = nowMs;
    return swipe;
}
