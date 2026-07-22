#ifndef HISTORY_H
#define HISTORY_H

#include <stdint.h>
#include <stdbool.h>

#define HISTORY_SIZE 10  


// ===========================================================
// some structs to store sensor data
// ===========================================================

typedef struct {
    uint16_t distance_mm;
    uint32_t timestamp_ms;
} sensor_sample_t;

typedef struct {
    sensor_sample_t samples[HISTORY_SIZE];
    uint8_t head;       
    uint8_t count;      
} sensor_history_t;

// ===========================================================

#ifdef __cplusplus
extern "C" {
#endif


uint16_t history_average(const sensor_history_t *hist, uint8_t n);

// Resets  history buffer
void history_init(sensor_history_t *hist);

// Add new sample
void history_push(sensor_history_t *hist, uint16_t distance_mm, uint32_t timestamp_ms);

// Gets the Nth most newest sample
bool history_get(const sensor_history_t *hist, uint8_t n, sensor_sample_t *out);


#ifdef __cplusplus
}
#endif

#endif