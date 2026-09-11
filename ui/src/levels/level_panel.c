#include "ui_app.h"
#include "ui_common.h"
#include "ui_levels.h"

#include "display/ui/widget/panel.h"

typedef struct PanelLevelState {
    UIExamplePage common;
    UUIPanel panel;
    UUILabel content;
} PanelLevelState;

static PanelLevelState state;

static bool panel_enter(UIExampleApp *app) {
    state = (PanelLevelState){ 0 };
    if (!ui_example_page_init(&state.common, NULL, "03 - PANEL",
                              "A decorative bounded container.",
                              "PANEL IS NOT FOCUSABLE")) {
        return false;
    }

    unsigned_ui_panel_init(&state.panel, ui_example_rect(40, 80, 240, 72),
                           UI_EXAMPLE_STYLE_DEFAULT);
    unsigned_ui_label_init(&state.content, ui_example_rect(0, 112, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "CONTENT INSIDE THE PANEL");

    if (!ui_example_page_add(&state.common, &state.panel.element) ||
        !ui_example_page_add(&state.common, &state.content.element)) {
        return false;
    }

    return unsigned_ui_push_page(&app->ui, &state.common.page) == U_UI_RESULT_OK;
}

const UIExampleLevel UI_EXAMPLE_LEVEL_PANEL = {
    .name = "Panel",
    .enter = panel_enter,
    .leave = NULL,
    .tick = NULL,
};
