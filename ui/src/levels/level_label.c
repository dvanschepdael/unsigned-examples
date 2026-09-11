#include "ui_app.h"
#include "ui_common.h"
#include "ui_levels.h"

#include "display/ui/widget/label.h"

typedef struct LabelLevelState {
    UIExamplePage common;
    UUILabel label;
    bool alternate_text;
} LabelLevelState;

static LabelLevelState state;

static bool label_enter(UIExampleApp *app) {
    state = (LabelLevelState){ 0 };
    if (!ui_example_page_init(&state.common, NULL, "01 - LABEL",
                              "Static text and runtime updates.",
                              "A: CHANGE TEXT")) {
        return false;
    }

    unsigned_ui_label_init(&state.label, ui_example_rect(0, 96, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "HELLO FROM UNSIGNED UI    ");
    if (!ui_example_page_add(&state.common, &state.label.element)) {
        return false;
    }

    return unsigned_ui_push_page(&app->ui, &state.common.page) == U_UI_RESULT_OK;
}

static void label_tick(UIExampleApp *app, const UInputController *controller) {
    (void)app;
    if (controller != NULL &&
        (controller->state.pressed & U_INPUT_BUTTON_A) != 0u) {
        state.alternate_text = !state.alternate_text;
        unsigned_ui_label_set_text(&state.label,
            state.alternate_text ? "TEXT CHANGED WITH SET_TEXT" : "HELLO FROM UNSIGNED UI    ");
    }
}

const UIExampleLevel UI_EXAMPLE_LEVEL_LABEL = {
    .name = "Label",
    .enter = label_enter,
    .leave = NULL,
    .tick = label_tick,
};
