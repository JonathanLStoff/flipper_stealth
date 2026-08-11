#pragma once

#include <furi.h>

// One history sample is drawn per pixel column inside the graph frame, so the
// buffer length is exactly the frame's inner width (see monitor_draw.c).
#define VITALS_HISTORY_LEN 78

// Plausible resting-adult ranges. Values random-walk inside these and turn
// around at the edges, so the display never parks on a boundary the way a
// clamp would - a reading pinned at exactly 96 for a minute reads as fake.
#define VITALS_HR_MIN  58
#define VITALS_HR_MAX  96
#define VITALS_SYS_MIN 105
#define VITALS_SYS_MAX 132
#define VITALS_DIA_MIN 66
#define VITALS_DIA_MAX 86

// Systolic and diastolic walk independently, which on its own would let them
// drift into a nonsense pairing (e.g. 106/86). After each step the diastolic
// is pulled back until the gap between them lands in this range.
#define VITALS_PULSE_PRESSURE_MIN 32
#define VITALS_PULSE_PRESSURE_MAX 56

typedef struct {
    uint8_t heart_rate;
    uint8_t systolic;
    uint8_t diastolic;

    // Heart-rate trend, oldest first. Shorter than VITALS_HISTORY_LEN until
    // the graph has filled from the left.
    uint8_t history[VITALS_HISTORY_LEN];
    uint8_t history_count;
} Vitals;

// Seeds every reading to a random point in its normal range.
void vitals_init(Vitals* vitals);

// Advances one update cycle: each reading moves 1-4 units up or down.
void vitals_tick(Vitals* vitals);

// Milliseconds until the next tick - a fresh 1000-2000ms each time, so the
// refresh doesn't beat with a metronome's regularity.
uint32_t vitals_next_interval_ms(void);

// Mean arterial pressure, the derived number a real monitor shows next to the
// cuff reading: diastolic + a third of the pulse pressure.
uint8_t vitals_mean_arterial_pressure(const Vitals* vitals);
