#include "ui_app.h"
#include "ui_common.h"
#include "ui_levels.h"

#include "display/ui/widget/button.h"

#include <stdio.h>

typedef struct ButtonLevelState {
    UIExamplePage common;
    UUIMenu menu;
    UUIButton button;
    UUILabel status;
    u16 press_count;
    char status_text[32];
} ButtonLevelState;

static ButtonLevelState state;

static void button_pressed(UUIButton *button, void *context) {
    ButtonLevelState *level = context;
    (void)button;

    if (level == NULL) {
        return;
    }

    ++level->press_count;
    (void)snprintf(level->status_text, sizeof(level->status_text), "PRESSED: %5u",
                   (unsigned int)level->press_count);
    unsigned_ui_label_set_text(&level->status, level->status_text);
}

static bool button_enter(UIExampleApp *app) {
    state = (ButtonLevelState){ 0 };
    unsigned_ui_menu_init(&state.menu);
    if (!ui_example_page_init(&state.common, &state.menu, "04 - BUTTON",
                              "Focusable action with a callback.",
                              "A: PRESS BUTTON")) {
        return false;
    }

    unsigned_ui_button_init(&state.button, ui_example_rect(40, 88, 240, 16),
                            UI_EXAMPLE_STYLE_DEFAULT, "PRESS A", button_pressed, &state);
    unsigned_ui_label_init(&state.status, ui_example_rect(0, 128, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "PRESSED:     0");

    if (!ui_example_page_add(&state.common, &state.button.element) ||
        !ui_example_page_add(&state.common, &state.status.element) ||
        unsigned_ui_menu_add(&state.menu, &state.button.element) != U_UI_RESULT_OK) {
        return false;
    }

    return unsigned_ui_push_page(&app->ui, &state.common.page) == U_UI_RESULT_OK;
}

const UIExampleLevel UI_EXAMPLE_LEVEL_BUTTON = {
    .name = "Button",
    .enter = button_enter,
    .leave = NULL,
    .tick = NULL,
};
