#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>

#include "vitals.h"

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* input_queue;

    // The draw callback runs on the GUI thread while vitals_tick() runs on the
    // app thread, so the model is mutex-guarded rather than shared bare.
    FuriMutex* mutex;
    Vitals vitals;

    bool running;
} VitalsApp;
