#include "ui_common.h"

#include "display/ui/widget/button.h"
#include "display/ui/widget/image.h"
#include "display/ui/widget/panel.h"
#include "display/ui/widget/progress_bar.h"
#include "system/fix.h"

#include <stdio.h>

#define UI_EXAMPLE_FIX_COLUMNS 40u
#define UI_EXAMPLE_CHAR_WIDTH 8u

static u8 ui_example_row(const UUIElement *element) {
    return element != NULL ? (u8)(element->bounds.y / UI_EXAMPLE_CHAR_WIDTH) : 0u;
}

static u8 ui_example_width_chars(const UUIElement *element) {
    if (element == NULL) {
        return 3u;
    }

    u16 width = element->bounds.width / UI_EXAMPLE_CHAR_WIDTH;
    if (width < 3u) {
        width = 3u;
    }
    if (width > UI_EXAMPLE_FIX_COLUMNS) {
        width = UI_EXAMPLE_FIX_COLUMNS;
    }
    return (u8)width;
}

static void ui_example_draw_panel(void *context, const UUITheme *theme,
                                  const UUIPanel *panel) {
    UNeoGeoUIRenderer *backend = context;
    (void)theme;

    if (backend == NULL || panel == NULL) {
        return;
    }

    const u8 width = ui_example_width_chars(&panel->element);
    const u8 top = ui_example_row(&panel->element);
    u8 rows = (u8)(panel->element.bounds.height / UI_EXAMPLE_CHAR_WIDTH);
    if (rows == 0u) {
        rows = 1u;
    }

    for (u8 row_offset = 0u; row_offset < rows; ++row_offset) {
        const bool border = row_offset == 0u || row_offset + 1u == rows;
        backend->text[0] = border ? '+' : '|';
        for (u8 i = 1u; i + 1u < width; ++i) {
            backend->text[i] = border ? '-' : ' ';
        }
        backend->text[width - 1u] = border ? '+' : '|';
        backend->text[width] = '\0';
        unsigned_neo_geo_fix_center_text((u8)(top + row_offset), backend->config.palette,
                                         backend->text);
    }
}

static void ui_example_draw_button(void *context, const UUITheme *theme,
                                   const UUIButton *button) {
    UNeoGeoUIRenderer *backend = context;
    (void)theme;

    if (backend == NULL || button == NULL) {
        return;
    }

    const char *text = button->text != NULL ? button->text : "";
    if (!button->element.enabled) {
        (void)snprintf(backend->text, sizeof(backend->text), "  ( %-18.18s )  ", text);
    } else if (button->element.focused) {
        (void)snprintf(backend->text, sizeof(backend->text), "> [ %-18.18s ] <", text);
    } else {
        (void)snprintf(backend->text, sizeof(backend->text), "  [ %-18.18s ]  ", text);
    }

    unsigned_neo_geo_fix_center_text(ui_example_row(&button->element), backend->config.palette,
                                     backend->text);
}

static void ui_example_draw_progress_bar(void *context, const UUITheme *theme,
                                         const UUIProgressBar *progress_bar) {
    UNeoGeoUIRenderer *backend = context;
    (void)theme;

    if (backend == NULL || progress_bar == NULL) {
        return;
    }

    enum { BAR_WIDTH = 20 };
    const u16 range = (u16)(progress_bar->max_value - progress_bar->min_value);
    const u16 offset = (u16)(progress_bar->value - progress_bar->min_value);
    const u8 filled = range == 0u ? BAR_WIDTH : (u8)(((u32)offset * BAR_WIDTH) / range);

    backend->text[0] = '[';
    for (u8 i = 0u; i < BAR_WIDTH; ++i) {
        backend->text[i + 1u] = i < filled ? '#' : '-';
    }
    backend->text[BAR_WIDTH + 1u] = ']';
    backend->text[BAR_WIDTH + 2u] = '\0';

    unsigned_neo_geo_fix_center_text(ui_example_row(&progress_bar->element),
                                     backend->config.palette, backend->text);
}

static void ui_example_draw_image(void *context, const UUITheme *theme,
                                  const UIImage *image) {
    UNeoGeoUIRenderer *backend = context;
    (void)theme;

    if (backend == NULL || image == NULL) {
        return;
    }

    (void)snprintf(backend->text, sizeof(backend->text), "[ IMAGE ASSET %03u ]",
                   (unsigned int)image->asset);
    unsigned_neo_geo_fix_center_text(ui_example_row(&image->element), backend->config.palette,
                                     backend->text);
}

UUIRect ui_example_rect(s16 x, s16 y, u16 width, u16 height) {
    return (UUIRect){
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };
}

bool ui_example_page_add(UIExamplePage *example, UUIElement *element) {
    return example != NULL &&
           unsigned_ui_screen_add(&example->screen, element) == U_UI_RESULT_OK;
}

bool ui_example_page_init(UIExamplePage *example, UUIMenu *menu, const char *title,
                          const char *description, const char *controls) {
    if (example == NULL) {
        return false;
    }

    *example = (UIExamplePage){ 0 };
    unsigned_ui_screen_init(&example->screen);

    unsigned_ui_label_init(&example->title, ui_example_rect(0, 16, 320, 16),
                           UI_EXAMPLE_STYLE_TITLE, title);
    unsigned_ui_label_init(&example->description, ui_example_rect(0, 48, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, description);
    unsigned_ui_label_init(&example->controls, ui_example_rect(0, 192, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, controls);
    unsigned_ui_label_init(&example->navigation, ui_example_rect(0, 208, 320, 8),
                           UI_EXAMPLE_STYLE_DEFAULT, "C/D: PREVIOUS/NEXT EXAMPLE");

    if (!ui_example_page_add(example, &example->title.element) ||
        !ui_example_page_add(example, &example->description.element) ||
        !ui_example_page_add(example, &example->controls.element) ||
        !ui_example_page_add(example, &example->navigation.element)) {
        return false;
    }

    unsigned_ui_page_init(&example->page, &example->screen, menu, false);
    return true;
}

void ui_example_renderer_init(UUIRenderer *renderer, UNeoGeoUIRenderer *backend,
                              UUITheme *theme) {
    const UNeoGeoUIRendererConfig config = {
        .tall_label_style = UI_EXAMPLE_STYLE_TITLE,
        .palette = 0u,
        .selector_option_width = 10u,
    };

    if (renderer == NULL || backend == NULL || theme == NULL) {
        return;
    }

    *theme = (UUITheme){ 0 };
    unsigned_neo_geo_ui_renderer_init(renderer, backend, theme, &config);

    /* Unsigned already renders labels/selectors on FIX. The example only fills the
     * callbacks that the generic Neo Geo backend intentionally leaves project-defined. */
    renderer->draw_panel = ui_example_draw_panel;
    renderer->draw_button = ui_example_draw_button;
    renderer->draw_progress_bar = ui_example_draw_progress_bar;
    renderer->draw_image = ui_example_draw_image;
}

void ui_example_input_from_controller(UUIInput *ui_input,
                                      const UInputController *controller) {
    if (ui_input == NULL) {
        return;
    }

    unsigned_ui_input_clear(ui_input);
    if (controller == NULL) {
        return;
    }

    const UInputMask pressed = controller->state.pressed;
    ui_input->navigate_previous_pressed = (pressed & U_INPUT_BUTTON_UP) != 0u;
    ui_input->navigate_next_pressed = (pressed & U_INPUT_BUTTON_DOWN) != 0u;
    ui_input->value_previous_pressed = (pressed & U_INPUT_BUTTON_LEFT) != 0u;
    ui_input->value_next_pressed = (pressed & U_INPUT_BUTTON_RIGHT) != 0u;
    ui_input->confirm_pressed = (pressed & U_INPUT_BUTTON_A) != 0u;
    ui_input->cancel_pressed = (pressed & U_INPUT_BUTTON_B) != 0u;
}
