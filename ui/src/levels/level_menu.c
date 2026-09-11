#include "ui_app.h"
#include "ui_common.h"
#include "ui_levels.h"

#include "display/ui/layout.h"
#include "display/ui/widget/button.h"

#include <stdio.h>

typedef struct MenuLevelState {
    UIExamplePage common;
    UUIMenu menu;
    UUIButton first;
    UUIButton disabled;
    UUIButton third;
    UUILabel status;
    char status_text[40];
} MenuLevelState;

static MenuLevelState state;

static void menu_set_status(const char *name) {
    (void)snprintf(state.status_text, sizeof(state.status_text), "SELECTED: %-8.8s", name);
    unsigned_ui_label_set_text(&state.status, state.status_text);
}

static void menu_first_pressed(UUIButton *button, void *context) {
    (void)button;
    (void)context;
    menu_set_status("FIRST");
}

static void menu_disabled_pressed(UUIButton *button, void *context) {
    (void)button;
    (void)context;
    menu_set_status("DISABLED");
}

static void menu_third_pressed(UUIButton *button, void *context) {
    (void)button;
    (void)context;
    menu_set_status("THIRD");
}

static bool menu_enter(UIExampleApp *app) {
    UUIElement *items[3];

    state = (MenuLevelState){ 0 };
    unsigned_ui_menu_init(&state.menu);
    if (!ui_example_page_init(&state.common, &state.menu, "09 - MENU",
                              "Focus skips disabled elements.",
                              "UP/DOWN + A: NAVIGATE")) {
        return false;
    }

    unsigned_ui_button_init(&state.first, ui_example_rect(40, 0, 240, 16),
                            UI_EXAMPLE_STYLE_DEFAULT, "FIRST", menu_first_pressed, NULL);
    unsigned_ui_button_init(&state.disabled, ui_example_rect(40, 0, 240, 16),
                            UI_EXAMPLE_STYLE_DEFAULT, "DISABLED", menu_disabled_pressed, NULL);
    unsigned_ui_button_init(&state.third, ui_example_rect(40, 0, 240, 16),
                            UI_EXAMPLE_STYLE_DEFAULT, "THIRD", menu_third_pressed, NULL);
    unsigned_ui_element_set_enabled(&state.disabled.element, false);

    items[0] = &state.first.element;
    items[1] = &state.disabled.element;
    items[2] = &state.third.element;
    unsigned_ui_layout_linear(items, 3u, 40, 72, U_UI_LAYOUT_VERTICAL, 4u);

    unsigned_ui_label_init(&state.status, ui_example_rect(0, 144, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "SELECTED: NONE    ");

    if (!ui_example_page_add(&state.common, &state.first.element) ||
        !ui_example_page_add(&state.common, &state.disabled.element) ||
        !ui_example_page_add(&state.common, &state.third.element) ||
        !ui_example_page_add(&state.common, &state.status.element) ||
        unsigned_ui_menu_add(&state.menu, &state.first.element) != U_UI_RESULT_OK ||
        unsigned_ui_menu_add(&state.menu, &state.disabled.element) != U_UI_RESULT_OK ||
        unsigned_ui_menu_add(&state.menu, &state.third.element) != U_UI_RESULT_OK) {
        return false;
    }

    return unsigned_ui_push_page(&app->ui, &state.common.page) == U_UI_RESULT_OK;
}

const UIExampleLevel UI_EXAMPLE_LEVEL_MENU = {
    .name = "Menu",
    .enter = menu_enter,
    .leave = NULL,
    .tick = NULL,
};
