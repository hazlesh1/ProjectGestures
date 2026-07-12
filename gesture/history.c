#include "history.h"

void history_init(sensor_history_t *hist) {
    hist->head = 0;
    hist->count = 0;
}

void history_push(sensor_history_t *hist, uint16_t distance_mm, uint32_t timestamp_ms) {
    hist->head = (hist->head + 1) % HISTORY_SIZE;
    hist->samples[hist->head].distance_mm = distance_mm;
    hist->samples[hist->head].timestamp_ms = timestamp_ms;

    if (hist->count < HISTORY_SIZE) {
        hist->count++;
    }
}

bool history_get(const sensor_history_t *hist, uint8_t n, sensor_sample_t *out) {
    if (n >= hist->count) {
        return false; // if asked for more
    }


    uint8_t index = (hist->head + HISTORY_SIZE - n) % HISTORY_SIZE;
    *out = hist->samples[index];
    return true;
}