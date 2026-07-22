#ifndef GESTURE_LOGIC_H
#define GESTURE_LOGIC_H

#include <stdint.h>


#define GESTURE_ZONE_MIN_MM 30
#define GESTURE_ZONE_MAX_MM 350 

typedef enum {
    GESTURE_NONE = 0,
    GESTURE_SWIPE_LEFT,
    GESTURE_SWIPE_RIGHT,
    GESTURE_HOVER,
    GESTURE_SWIPE_UP,    
    GESTURE_SWIPE_DOWN,  
} gesture_event_t;

#ifdef __cplusplus
extern "C" {
#endif

void gesture_logic_init(void);

gesture_event_t gesture_logic_update(uint16_t left_mm, uint16_t right_mm,
                                     uint16_t up_mm, uint16_t down_mm,
                                     uint32_t timestamp_ms);

#ifdef __cplusplus
}
#endif

#endif 
