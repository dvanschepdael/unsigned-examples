#include "ui_app.h"

#include "ui_common.h"
#include "ui_levels.h"

#include "system/fix.h"

static void ui_example_app_clear_pages(UIExampleApp *app) {
    if (app == NULL) {
        return;
    }

    while (app->ui.page_count > 0u) {
        (void)unsigned_ui_pop_page(&app->ui);
    }
}

static void ui_example_app_leave_current(UIExampleApp *app) {
    const UIExampleLevel *level;

    if (app == NULL) {
        return;
    }

    level = ui_example_level_at(app->level_index);
    if (level != NULL && level->leave != NULL) {
        level->leave(app);
    }
    ui_example_app_clear_pages(app);
}

static bool ui_example_app_enter(UIExampleApp *app, u8 index) {
    const UIExampleLevel *level = ui_example_level_at(index);
    if (app == NULL || level == NULL || level->enter == NULL) {
        return false;
    }

    ui_example_app_leave_current(app);
    unsigned_neo_geo_fix_clear();
    app->level_index = index;
    return level->enter(app);
}

static void ui_example_app_change_level(UIExampleApp *app, bool next) {
    const u8 count = ui_example_level_count();
    u8 index;

    if (app == NULL || count == 0u) {
        return;
    }

    if (next) {
        index = (u8)((app->level_index + 1u) % count);
    } else {
        index = app->level_index == 0u ? (u8)(count - 1u) : (u8)(app->level_index - 1u);
    }

    (void)ui_example_app_enter(app, index);
}

bool ui_example_app_initialize(void *context) {
    UIExampleApp *app = context;
    if (app == NULL) {
        return false;
    }

    *app = (UIExampleApp){ 0 };
    if (!unsigned_input_manager_init(&app->input, 1u)) {
        return false;
    }

    ui_example_renderer_init(&app->renderer, &app->renderer_backend, &app->theme);
    unsigned_ui_context_init(&app->ui, &app->renderer);
    app->phase = U_NEO_GEO_PHASE_ATTRACT;

    return ui_example_app_enter(app, 0u);
}

void ui_example_app_shutdown(void *context) {
    UIExampleApp *app = context;
    if (app == NULL) {
        return;
    }

    ui_example_app_leave_current(app);
    unsigned_neo_geo_fix_clear();
}

void ui_example_app_tick(void *context) {
    UIExampleApp *app = context;
    UUIInput ui_input;
    const UIExampleLevel *level;
    const UInputController *controller;

    if (app == NULL || app->input.player_count == 0u) {
        return;
    }

    controller = &app->input.players[0];
    if ((controller->state.pressed & U_INPUT_BUTTON_C) != 0u) {
        ui_example_app_change_level(app, false);
        return;
    }
    if ((controller->state.pressed & U_INPUT_BUTTON_D) != 0u) {
        ui_example_app_change_level(app, true);
        return;
    }

    ui_example_input_from_controller(&ui_input, controller);
    unsigned_ui_update(&app->ui, &ui_input);

    level = ui_example_level_at(app->level_index);
    if (level != NULL && level->tick != NULL) {
        level->tick(app, controller);
    }
}

void ui_example_app_render(void *context) {
    UIExampleApp *app = context;
    if (app != NULL) {
        unsigned_ui_render(&app->ui);
    }
}

void ui_example_app_enter_phase(void *context, UNeoGeoPhase phase) {
    UIExampleApp *app = context;
    if (app != NULL) {
        app->phase = phase;
    }
}
