#include "ui_app.h"
#include "ui_common.h"
#include "ui_levels.h"

#include "display/ui/widget/progress_bar.h"

#include <stdio.h>

typedef struct ProgressBarLevelState {
    UIExamplePage common;
    UUIProgressBar progress;
    UUILabel status;
    char status_text[32];
} ProgressBarLevelState;

static ProgressBarLevelState state;

static void progress_update_status(void) {
    (void)snprintf(state.status_text, sizeof(state.status_text), "VALUE: %3u / 100",
                   (unsigned int)unsigned_ui_progress_bar_value(&state.progress));
    unsigned_ui_label_set_text(&state.status, state.status_text);
}

static bool progress_enter(UIExampleApp *app) {
    state = (ProgressBarLevelState){ 0 };
    if (!ui_example_page_init(&state.common, NULL, "05 - PROGRESS BAR",
                              "A bounded numeric progress value.",
                              "LEFT/RIGHT: CHANGE VALUE")) {
        return false;
    }

    unsigned_ui_progress_bar_init(&state.progress, ui_example_rect(64, 88, 192, 16),
                                  UI_EXAMPLE_STYLE_DEFAULT, 0u, 100u, 50u);
    unsigned_ui_label_init(&state.status, ui_example_rect(0, 128, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "VALUE:  50 / 100");

    if (!ui_example_page_add(&state.common, &state.progress.element) ||
        !ui_example_page_add(&state.common, &state.status.element)) {
        return false;
    }

    return unsigned_ui_push_page(&app->ui, &state.common.page) == U_UI_RESULT_OK;
}

static void progress_tick(UIExampleApp *app, const UInputController *controller) {
    u16 value;
    (void)app;

    if (controller == NULL) {
        return;
    }

    value = unsigned_ui_progress_bar_value(&state.progress);
    if ((controller->state.pressed & U_INPUT_BUTTON_LEFT) != 0u) {
        unsigned_ui_progress_bar_set_value(&state.progress, value >= 10u ? (u16)(value - 10u) : 0u);
        progress_update_status();
    } else if ((controller->state.pressed & U_INPUT_BUTTON_RIGHT) != 0u) {
        unsigned_ui_progress_bar_set_value(&state.progress, value <= 90u ? (u16)(value + 10u) : 100u);
        progress_update_status();
    }
}

const UIExampleLevel UI_EXAMPLE_LEVEL_PROGRESS_BAR = {
    .name = "Progress bar",
    .enter = progress_enter,
    .leave = NULL,
    .tick = progress_tick,
};
