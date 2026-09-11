#include "ui_app.h"

#include "system/runtime.h"

static int run_ui_examples(bool mvs_title) {
    UIExampleApp app = { 0 };
    const UNeoGeoRuntimeDefinition runtime = {
        .context = &app,
        .input = &app.input,
        .initialize = ui_example_app_initialize,
        .shutdown = ui_example_app_shutdown,
        .tick = ui_example_app_tick,
        .render = ui_example_app_render,
        .enter_phase = ui_example_app_enter_phase,
    };

    return mvs_title
        ? unsigned_neo_geo_main_mvs(&runtime, 0u)
        : unsigned_neo_geo_main(&runtime, 0u);
}

int main(void) {
    return run_ui_examples(false);
}

int main_mvs_title(void) {
    return run_ui_examples(true);
}
