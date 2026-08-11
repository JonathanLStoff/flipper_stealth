#pragma once

#include <gui/gui.h>

// ViewPort draw callback. `ctx` is the VitalsApp; the callback takes the app's
// mutex itself, since it runs on the GUI thread.
void monitor_draw_callback(Canvas* canvas, void* ctx);
