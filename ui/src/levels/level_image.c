#include "ui_app.h"
#include "ui_common.h"
#include "ui_levels.h"

#include "display/ui/widget/image.h"

#include <stdio.h>

typedef struct ImageLevelState {
    UIExamplePage common;
    UIImage image;
    UUILabel status;
    char status_text[32];
} ImageLevelState;

static ImageLevelState state;

static void image_set_asset(UUIAssetId asset) {
    unsigned_ui_image_set_asset(&state.image, asset);
    (void)snprintf(state.status_text, sizeof(state.status_text), "ASSET ID: %u",
                   (unsigned int)asset);
    unsigned_ui_label_set_text(&state.status, state.status_text);
}

static bool image_enter(UIExampleApp *app) {
    state = (ImageLevelState){ 0 };
    if (!ui_example_page_init(&state.common, NULL, "07 - IMAGE",
                              "Image widget carries an asset id.",
                              "LEFT/RIGHT: CHANGE ASSET ID")) {
        return false;
    }

    unsigned_ui_image_init(&state.image, ui_example_rect(64, 88, 192, 16),
                           UI_EXAMPLE_STYLE_DEFAULT, 1u);
    unsigned_ui_label_init(&state.status, ui_example_rect(0, 128, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "ASSET ID: 1");

    if (!ui_example_page_add(&state.common, &state.image.element) ||
        !ui_example_page_add(&state.common, &state.status.element)) {
        return false;
    }

    return unsigned_ui_push_page(&app->ui, &state.common.page) == U_UI_RESULT_OK;
}

static void image_tick(UIExampleApp *app, const UInputController *controller) {
    UUIAssetId asset;
    (void)app;

    if (controller == NULL) {
        return;
    }

    asset = state.image.asset;
    if ((controller->state.pressed & U_INPUT_BUTTON_LEFT) != 0u) {
        image_set_asset(asset <= 1u ? 3u : (UUIAssetId)(asset - 1u));
    } else if ((controller->state.pressed & U_INPUT_BUTTON_RIGHT) != 0u) {
        image_set_asset(asset >= 3u ? 1u : (UUIAssetId)(asset + 1u));
    }
}

const UIExampleLevel UI_EXAMPLE_LEVEL_IMAGE = {
    .name = "Image",
    .enter = image_enter,
    .leave = NULL,
    .tick = image_tick,
};
