#include "ui_app.h"
#include "ui_common.h"
#include "ui_levels.h"

#include "display/ui/widget/blink_label.h"

typedef struct BlinkLabelLevelState {
    UIExamplePage common;
    UUIBlinkLabel label;
} BlinkLabelLevelState;

static BlinkLabelLevelState state;

static bool blink_label_enter(UIExampleApp *app) {
    state = (BlinkLabelLevelState){ 0 };
    if (!ui_example_page_init(&state.common, NULL, "02 - BLINK LABEL",
                              "Frame-based blinking text.",
                              "NO INPUT REQUIRED")) {
        return false;
    }

    unsigned_ui_blink_label_init(&state.label, ui_example_rect(0, 96, 320, 8),
                                 UI_EXAMPLE_STYLE_DEFAULT, "PRESS START", 30u, true, 11u);
    if (!ui_example_page_add(&state.common, &state.label.label.element)) {
        return false;
    }

    return unsigned_ui_push_page(&app->ui, &state.common.page) == U_UI_RESULT_OK;
}

static void blink_label_tick(UIExampleApp *app, const UInputController *controller) {
    (void)app;
    (void)controller;
    unsigned_ui_blink_label_tick(&state.label);
}

const UIExampleLevel UI_EXAMPLE_LEVEL_BLINK_LABEL = {
    .name = "Blink label",
    .enter = blink_label_enter,
    .leave = NULL,
    .tick = blink_label_tick,
};
