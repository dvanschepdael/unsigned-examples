#include "ui_app.h"
#include "ui_common.h"
#include "ui_levels.h"

#include "display/ui/layout.h"

typedef struct LayoutLevelState {
    UIExamplePage common;
    UUILabel first;
    UUILabel second;
    UUILabel third;
} LayoutLevelState;

static LayoutLevelState state;

static bool layout_enter(UIExampleApp *app) {
    UUIElement *items[3];

    state = (LayoutLevelState){ 0 };
    if (!ui_example_page_init(&state.common, NULL, "08 - LINEAR LAYOUT",
                              "Layout computes element positions.",
                              "VERTICAL, 8 PIXELS SPACING")) {
        return false;
    }

    unsigned_ui_label_init(&state.first, ui_example_rect(0, 0, 160, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "FIRST ITEM");
    unsigned_ui_label_init(&state.second, ui_example_rect(0, 0, 160, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "SECOND ITEM");
    unsigned_ui_label_init(&state.third, ui_example_rect(0, 0, 160, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "THIRD ITEM");

    items[0] = &state.first.element;
    items[1] = &state.second.element;
    items[2] = &state.third.element;
    unsigned_ui_layout_linear(items, 3u, 80, 80, U_UI_LAYOUT_VERTICAL, 8u);

    if (!ui_example_page_add(&state.common, &state.first.element) ||
        !ui_example_page_add(&state.common, &state.second.element) ||
        !ui_example_page_add(&state.common, &state.third.element)) {
        return false;
    }

    return unsigned_ui_push_page(&app->ui, &state.common.page) == U_UI_RESULT_OK;
}

const UIExampleLevel UI_EXAMPLE_LEVEL_LAYOUT = {
    .name = "Linear layout",
    .enter = layout_enter,
    .leave = NULL,
    .tick = NULL,
};
