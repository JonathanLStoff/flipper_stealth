#include "vitals.h"

#include <furi_hal_random.h>
#include <string.h>

static uint32_t vitals_random_below(uint32_t bound) {
    return furi_hal_random_get() % bound;
}

static uint8_t vitals_random_in_range(uint8_t min, uint8_t max) {
    return (uint8_t)(min + vitals_random_below((uint32_t)(max - min) + 1));
}

// Moves `value` by 1-4 in a random direction. A step that would leave the
// range turns around and walks the same distance the other way, rather than
// reflecting off the edge - reflecting can land back on the value it started
// from (e.g. min+1 stepping -2), and a reading that redraws unchanged reads as
// a frozen display. Turning around always moves, and is safe for any range
// wider than twice the maximum step, which every range in vitals.h is.
static uint8_t vitals_step(uint8_t value, uint8_t min, uint8_t max) {
    uint32_t r = furi_hal_random_get();
    int32_t step = (int32_t)(r % 4) + 1; // 1-4
    if(r & 0x80000000UL) step = -step; // direction from a bit the modulo didn't use

    int32_t next = (int32_t)value + step;
    if(next < min || next > max) next = (int32_t)value - step;
    return (uint8_t)next;
}

static void vitals_history_push(Vitals* vitals, uint8_t sample) {
    if(vitals->history_count < VITALS_HISTORY_LEN) {
        vitals->history[vitals->history_count++] = sample;
        return;
    }
    // Full: scroll left, dropping the oldest sample. 78 bytes once or twice a
    // second is cheaper than the bookkeeping a ring buffer would need here.
    memmove(vitals->history, vitals->history + 1, VITALS_HISTORY_LEN - 1);
    vitals->history[VITALS_HISTORY_LEN - 1] = sample;
}

// Keeps the gap between systolic and diastolic clinically plausible by moving
// the diastolic, since the systolic is the number that draws the eye.
static void vitals_reconcile_pressures(Vitals* vitals) {
    int32_t gap = (int32_t)vitals->systolic - (int32_t)vitals->diastolic;
    if(gap < VITALS_PULSE_PRESSURE_MIN) {
        vitals->diastolic = (uint8_t)(vitals->systolic - VITALS_PULSE_PRESSURE_MIN);
    } else if(gap > VITALS_PULSE_PRESSURE_MAX) {
        vitals->diastolic = (uint8_t)(vitals->systolic - VITALS_PULSE_PRESSURE_MAX);
    }

    if(vitals->diastolic < VITALS_DIA_MIN) vitals->diastolic = VITALS_DIA_MIN;
    if(vitals->diastolic > VITALS_DIA_MAX) vitals->diastolic = VITALS_DIA_MAX;
}

void vitals_init(Vitals* vitals) {
    memset(vitals, 0, sizeof(Vitals));

    vitals->heart_rate = vitals_random_in_range(VITALS_HR_MIN, VITALS_HR_MAX);
    vitals->systolic = vitals_random_in_range(VITALS_SYS_MIN, VITALS_SYS_MAX);
    vitals->diastolic = vitals_random_in_range(VITALS_DIA_MIN, VITALS_DIA_MAX);
    vitals_reconcile_pressures(vitals);

    vitals_history_push(vitals, vitals->heart_rate);
}

void vitals_tick(Vitals* vitals) {
    vitals->heart_rate = vitals_step(vitals->heart_rate, VITALS_HR_MIN, VITALS_HR_MAX);
    vitals->systolic = vitals_step(vitals->systolic, VITALS_SYS_MIN, VITALS_SYS_MAX);
    vitals->diastolic = vitals_step(vitals->diastolic, VITALS_DIA_MIN, VITALS_DIA_MAX);
    vitals_reconcile_pressures(vitals);

    vitals_history_push(vitals, vitals->heart_rate);
}

uint32_t vitals_next_interval_ms(void) {
    return 1000 + vitals_random_below(1001); // 1000-2000ms
}

uint8_t vitals_mean_arterial_pressure(const Vitals* vitals) {
    return (uint8_t)(vitals->diastolic + (vitals->systolic - vitals->diastolic) / 3);
}
