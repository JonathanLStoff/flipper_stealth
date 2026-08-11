#include "stealth_i.h"
#include "monitor_draw.h"

#include <stdlib.h>
#include <string.h>

static void stealth_input_callback(InputEvent* event, void* ctx) {
    FuriMessageQueue* input_queue = ctx;
    furi_message_queue_put(input_queue, event, FuriWaitForever);
}

static VitalsApp* stealth_app_alloc(void) {
    VitalsApp* app = malloc(sizeof(VitalsApp));
    memset(app, 0, sizeof(VitalsApp));

    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    vitals_init(&app->vitals);
    app->running = true;

    app->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, monitor_draw_callback, app);
    view_port_input_callback_set(app->view_port, stealth_input_callback, app->input_queue);

    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    return app;
}

static void stealth_app_free(VitalsApp* app) {
    view_port_enabled_set(app->view_port, false);
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);

    furi_message_queue_free(app->input_queue);
    furi_mutex_free(app->mutex);

    free(app);
}

int32_t stealth_app(void* p) {
    UNUSED(p);
    VitalsApp* app = stealth_app_alloc();

    // A deadline rather than a fixed wait: input arriving mid-cycle wakes the
    // queue early, and restarting the full interval each time would let a
    // button-masher stall the readings indefinitely.
    uint32_t deadline = furi_get_tick() + furi_ms_to_ticks(vitals_next_interval_ms());

    while(app->running) {
        // Signed difference, not `deadline > now`: the tick counter is a
        // uint32_t that wraps roughly every 49 days, and a plain comparison
        // across that wrap would ask for a ~49-day wait.
        int32_t remaining = (int32_t)(deadline - furi_get_tick());
        uint32_t wait = (remaining > 0) ? (uint32_t)remaining : 0;

        InputEvent event;
        if(furi_message_queue_get(app->input_queue, &event, wait) == FuriStatusOk) {
            if(event.key == InputKeyBack &&
               (event.type == InputTypeShort || event.type == InputTypeLong)) {
                app->running = false;
            }
            continue;
        }

        furi_mutex_acquire(app->mutex, FuriWaitForever);
        vitals_tick(&app->vitals);
        furi_mutex_release(app->mutex);

        view_port_update(app->view_port);
        deadline = furi_get_tick() + furi_ms_to_ticks(vitals_next_interval_ms());
    }

    stealth_app_free(app);
    return 0;
}
