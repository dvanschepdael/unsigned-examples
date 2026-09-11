#include "ui_app.h"
#include "ui_common.h"
#include "ui_levels.h"

#include "display/ui/widget/button.h"
#include "display/ui/widget/panel.h"

typedef struct PagesLevelState {
    UIExampleApp *app;
    UIExamplePage root;
    UUIMenu root_menu;
    UUIButton open_button;

    UUIScreen overlay_screen;
    UUIPage overlay_page;
    UUIMenu overlay_menu;
    UUIPanel overlay_panel;
    UUILabel overlay_label;
    UUIButton close_button;
} PagesLevelState;

static PagesLevelState state;

static void pages_close_overlay(void) {
    if (state.app == NULL) {
        return;
    }

    if (unsigned_ui_current_page(&state.app->ui) == &state.overlay_page) {
        (void)unsigned_ui_pop_page(&state.app->ui);
    }
}

static void pages_close_pressed(UUIButton *button, void *context) {
    (void)button;
    (void)context;
    pages_close_overlay();
}

static void pages_cancel(void *context) {
    (void)context;
    pages_close_overlay();
}

static void pages_open_pressed(UUIButton *button, void *context) {
    (void)button;
    (void)context;

    if (state.app != NULL &&
        unsigned_ui_current_page(&state.app->ui) == &state.root.page) {
        (void)unsigned_ui_push_page(&state.app->ui, &state.overlay_page);
    }
}

static bool pages_enter(UIExampleApp *app) {
    state = (PagesLevelState){ 0 };
    state.app = app;

    unsigned_ui_menu_init(&state.root_menu);
    if (!ui_example_page_init(&state.root, &state.root_menu, "10 - PAGE STACK",
                              "Push and pop an overlay page.",
                              "A: OPEN   B: CLOSE OVERLAY")) {
        return false;
    }

    unsigned_ui_button_init(&state.open_button, ui_example_rect(40, 96, 240, 16),
                            UI_EXAMPLE_STYLE_DEFAULT, "OPEN OVERLAY", pages_open_pressed, NULL);
    if (!ui_example_page_add(&state.root, &state.open_button.element) ||
        unsigned_ui_menu_add(&state.root_menu, &state.open_button.element) != U_UI_RESULT_OK) {
        return false;
    }

    unsigned_ui_screen_init(&state.overlay_screen);
    unsigned_ui_menu_init(&state.overlay_menu);
    unsigned_ui_menu_set_cancel_handler(&state.overlay_menu, pages_cancel, NULL);

    unsigned_ui_panel_init(&state.overlay_panel, ui_example_rect(40, 72, 240, 96),
                           UI_EXAMPLE_STYLE_DEFAULT);
    unsigned_ui_label_init(&state.overlay_label, ui_example_rect(0, 96, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "THIS PAGE IS AN OVERLAY");
    unsigned_ui_button_init(&state.close_button, ui_example_rect(40, 128, 240, 16),
                            UI_EXAMPLE_STYLE_DEFAULT, "CLOSE", pages_close_pressed, NULL);

    if (unsigned_ui_screen_add(&state.overlay_screen, &state.overlay_panel.element) != U_UI_RESULT_OK ||
        unsigned_ui_screen_add(&state.overlay_screen, &state.overlay_label.element) != U_UI_RESULT_OK ||
        unsigned_ui_screen_add(&state.overlay_screen, &state.close_button.element) != U_UI_RESULT_OK ||
        unsigned_ui_menu_add(&state.overlay_menu, &state.close_button.element) != U_UI_RESULT_OK) {
        return false;
    }

    unsigned_ui_page_init(&state.overlay_page, &state.overlay_screen, &state.overlay_menu, true);
    return unsigned_ui_push_page(&app->ui, &state.root.page) == U_UI_RESULT_OK;
}

static void pages_leave(UIExampleApp *app) {
    (void)app;
    state.app = NULL;
}

const UIExampleLevel UI_EXAMPLE_LEVEL_PAGES = {
    .name = "Page stack",
    .enter = pages_enter,
    .leave = pages_leave,
    .tick = NULL,
};
