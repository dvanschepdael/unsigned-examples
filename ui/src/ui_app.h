#ifndef UI_EXAMPLE_APP_H
#define UI_EXAMPLE_APP_H

#include "display/ui/context.h"
#include "input/input.h"
#include "system/runtime.h"
#include "renderer/ui_renderer.h"

typedef struct UIExampleApp {
    UInputManager input;
    UUITheme theme;
    UUIRenderer renderer;
    UNeoGeoUIRenderer renderer_backend;
    UUIContext ui;
    UNeoGeoPhase phase;
    u8 level_index;
} UIExampleApp;

bool ui_example_app_initialize(void *context);
void ui_example_app_shutdown(void *context);
void ui_example_app_tick(void *context);
void ui_example_app_render(void *context);
void ui_example_app_enter_phase(void *context, UNeoGeoPhase phase);

#endif
