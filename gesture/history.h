#ifndef SENSOR_HISTORY_H
#define SENSOR_HISTORY_H

#include <stdint.h>
#include <stdbool.h>

#define HISTORY_SIZE 10  // Past samples per sensor 

typedef struct {
    uint16_t distance_mm;
    uint32_t timestamp_ms;
} sensor_sample_t;

typedef struct {
    sensor_sample_t samples[HISTORY_SIZE];
    uint8_t head;       
    uint8_t count;      // num valid samples we have so far 
} sensor_history_t;

#ifdef __cplusplus
extern "C" {
#endif

// Resets  history buffer
void history_init(sensor_history_t *hist);

// Adds a new sample (overwrites oldest if buffer  full)
void history_push(sensor_history_t *hist, uint16_t distance_mm, uint32_t timestamp_ms);

bool history_get(const sensor_history_t *hist, uint8_t n, sensor_sample_t *out);

#ifdef __cplusplus
}
#endif

#endif 