#include "ui_levels.h"

extern const UIExampleLevel UI_EXAMPLE_LEVEL_LABEL;
extern const UIExampleLevel UI_EXAMPLE_LEVEL_BLINK_LABEL;
extern const UIExampleLevel UI_EXAMPLE_LEVEL_PANEL;
extern const UIExampleLevel UI_EXAMPLE_LEVEL_BUTTON;
extern const UIExampleLevel UI_EXAMPLE_LEVEL_PROGRESS_BAR;
extern const UIExampleLevel UI_EXAMPLE_LEVEL_SELECTOR;
extern const UIExampleLevel UI_EXAMPLE_LEVEL_IMAGE;
extern const UIExampleLevel UI_EXAMPLE_LEVEL_LAYOUT;
extern const UIExampleLevel UI_EXAMPLE_LEVEL_MENU;
extern const UIExampleLevel UI_EXAMPLE_LEVEL_PAGES;

static const UIExampleLevel *const LEVELS[] = {
    &UI_EXAMPLE_LEVEL_LABEL,
    &UI_EXAMPLE_LEVEL_BLINK_LABEL,
    &UI_EXAMPLE_LEVEL_PANEL,
    &UI_EXAMPLE_LEVEL_BUTTON,
    &UI_EXAMPLE_LEVEL_PROGRESS_BAR,
    &UI_EXAMPLE_LEVEL_SELECTOR,
    &UI_EXAMPLE_LEVEL_IMAGE,
    &UI_EXAMPLE_LEVEL_LAYOUT,
    &UI_EXAMPLE_LEVEL_MENU,
    &UI_EXAMPLE_LEVEL_PAGES,
};

u8 ui_example_level_count(void) {
    return (u8)(sizeof(LEVELS) / sizeof(LEVELS[0]));
}

const UIExampleLevel *ui_example_level_at(u8 index) {
    return index < ui_example_level_count() ? LEVELS[index] : NULL;
}
