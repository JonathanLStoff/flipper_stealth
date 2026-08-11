#include "monitor_draw.h"
#include "stealth_i.h"

#include <stdio.h>

// The 128x64 screen is split into a wide left pane (heart rate + trend graph)
// and a narrow right pane (cuff pressure), separated by a vertical rule.
#define PANE_DIVIDER_X   84
#define HEADER_BASELINE  8
#define HEADER_RULE_Y    11

#define LEFT_CENTER_X    42
#define RIGHT_CENTER_X   107

// Graph frame; the trace is drawn one sample per pixel column inside it, which
// is why VITALS_HISTORY_LEN matches the inner width.
#define GRAPH_X 2
#define GRAPH_Y 32
#define GRAPH_W 80
#define GRAPH_H 30

#define GRAPH_INNER_X      (GRAPH_X + 1)
#define GRAPH_INNER_BOTTOM (GRAPH_Y + GRAPH_H - 2)
#define GRAPH_INNER_H      (GRAPH_H - 2)

// The trend is plotted against a slightly wider window than the heart rate can
// actually reach, so the trace never rides flush against the frame.
#define GRAPH_SCALE_MIN (VITALS_HR_MIN - 2)
#define GRAPH_SCALE_MAX (VITALS_HR_MAX + 2)

static uint8_t monitor_sample_to_y(uint8_t sample) {
    int32_t value = sample;
    if(value < GRAPH_SCALE_MIN) value = GRAPH_SCALE_MIN;
    if(value > GRAPH_SCALE_MAX) value = GRAPH_SCALE_MAX;

    int32_t offset = ((value - GRAPH_SCALE_MIN) * (GRAPH_INNER_H - 1)) /
                     (GRAPH_SCALE_MAX - GRAPH_SCALE_MIN);
    return (uint8_t)(GRAPH_INNER_BOTTOM - offset);
}

static void monitor_draw_graph(Canvas* canvas, const Vitals* vitals) {
    canvas_draw_frame(canvas, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H);

    if(vitals->history_count == 0) return;

    if(vitals->history_count == 1) {
        canvas_draw_dot(canvas, GRAPH_INNER_X, monitor_sample_to_y(vitals->history[0]));
        return;
    }

    for(uint8_t i = 1; i < vitals->history_count; i++) {
        canvas_draw_line(
            canvas,
            GRAPH_INNER_X + i - 1,
            monitor_sample_to_y(vitals->history[i - 1]),
            GRAPH_INNER_X + i,
            monitor_sample_to_y(vitals->history[i]));
    }

    // Mark the newest reading so the eye can find the live end of the trace.
    uint8_t last = vitals->history_count - 1;
    canvas_draw_box(canvas, GRAPH_INNER_X + last - 1, monitor_sample_to_y(vitals->history[last]) - 1, 2, 2);
}

static void monitor_draw_heart_rate(Canvas* canvas, const Vitals* vitals) {
    char text[8];

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 2, HEADER_BASELINE, AlignLeft, AlignBottom, "HR");
    canvas_draw_str_aligned(
        canvas, PANE_DIVIDER_X - 3, HEADER_BASELINE, AlignRight, AlignBottom, "bpm");
    canvas_draw_line(canvas, 0, HEADER_RULE_Y, PANE_DIVIDER_X - 2, HEADER_RULE_Y);

    snprintf(text, sizeof(text), "%u", (unsigned)vitals->heart_rate);
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str_aligned(canvas, LEFT_CENTER_X, 22, AlignCenter, AlignCenter, text);

    monitor_draw_graph(canvas, vitals);
}

static void monitor_draw_blood_pressure(Canvas* canvas, const Vitals* vitals) {
    char text[12];

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(
        canvas, RIGHT_CENTER_X, HEADER_BASELINE, AlignCenter, AlignBottom, "NIBP");
    canvas_draw_line(canvas, PANE_DIVIDER_X + 2, HEADER_RULE_Y, 127, HEADER_RULE_Y);

    snprintf(text, sizeof(text), "%u", (unsigned)vitals->systolic);
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str_aligned(canvas, RIGHT_CENTER_X, 22, AlignCenter, AlignCenter, text);

    snprintf(text, sizeof(text), "/%u", (unsigned)vitals->diastolic);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, RIGHT_CENTER_X, 41, AlignCenter, AlignBottom, text);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, RIGHT_CENTER_X, 51, AlignCenter, AlignBottom, "mmHg");

    snprintf(text, sizeof(text), "MAP %u", (unsigned)vitals_mean_arterial_pressure(vitals));
    canvas_draw_str_aligned(canvas, RIGHT_CENTER_X, 61, AlignCenter, AlignBottom, text);
}

void monitor_draw_callback(Canvas* canvas, void* ctx) {
    VitalsApp* app = ctx;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    furi_mutex_acquire(app->mutex, FuriWaitForever);
    Vitals vitals = app->vitals;
    furi_mutex_release(app->mutex);

    monitor_draw_heart_rate(canvas, &vitals);
    canvas_draw_line(canvas, PANE_DIVIDER_X, 0, PANE_DIVIDER_X, 63);
    monitor_draw_blood_pressure(canvas, &vitals);
}
