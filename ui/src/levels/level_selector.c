#include "ui_app.h"
#include "ui_common.h"
#include "ui_levels.h"

#include "display/ui/widget/selector.h"

#include <stdio.h>

typedef struct SelectorLevelState {
    UIExamplePage common;
    UUIMenu menu;
    UUISelector selector;
    UUILabel status;
    char status_text[40];
} SelectorLevelState;

static SelectorLevelState state;
static const char *const OPTIONS[] = { "EASY", "NORMAL", "HARD" };

static void selector_changed(UUISelector *selector, u8 selected_index, void *context) {
    SelectorLevelState *level = context;
    (void)selected_index;

    if (level == NULL || selector == NULL) {
        return;
    }

    (void)snprintf(level->status_text, sizeof(level->status_text), "SELECTED: %s",
                   unsigned_ui_selector_selected_text(selector));
    unsigned_ui_label_set_text(&level->status, level->status_text);
}

static bool selector_enter(UIExampleApp *app) {
    state = (SelectorLevelState){ 0 };
    unsigned_ui_menu_init(&state.menu);
    if (!ui_example_page_init(&state.common, &state.menu, "06 - SELECTOR",
                              "Focusable list of options.",
                              "LEFT/RIGHT: CHANGE OPTION")) {
        return false;
    }

    unsigned_ui_selector_init(&state.selector, ui_example_rect(24, 88, 272, 16),
                              UI_EXAMPLE_STYLE_DEFAULT, "DIFFICULTY", OPTIONS, 3u, 1u, true,
                              selector_changed, &state);
    unsigned_ui_label_init(&state.status, ui_example_rect(0, 128, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "SELECTED: NORMAL");

    if (!ui_example_page_add(&state.common, &state.selector.element) ||
        !ui_example_page_add(&state.common, &state.status.element) ||
        unsigned_ui_menu_add(&state.menu, &state.selector.element) != U_UI_RESULT_OK) {
        return false;
    }

    return unsigned_ui_push_page(&app->ui, &state.common.page) == U_UI_RESULT_OK;
}

const UIExampleLevel UI_EXAMPLE_LEVEL_SELECTOR = {
    .name = "Selector",
    .enter = selector_enter,
    .leave = NULL,
    .tick = NULL,
};
