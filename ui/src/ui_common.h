#ifndef UI_EXAMPLE_COMMON_H
#define UI_EXAMPLE_COMMON_H
#include <stddef.h>

#include "display/ui/input.h"
#include "display/ui/page.h"
#include "display/ui/widget/label.h"
#include "input/input.h"
#include "renderer/ui_renderer.h"

enum {
    UI_EXAMPLE_STYLE_DEFAULT = 0,
    UI_EXAMPLE_STYLE_TITLE = 1,
};

typedef struct UIExamplePage {
    UUIScreen screen;
    UUIPage page;
    UUILabel title;
    UUILabel description;
    UUILabel controls;
    UUILabel navigation;
} UIExamplePage;

UUIRect ui_example_rect(s16 x, s16 y, u16 width, u16 height);

bool ui_example_page_init(UIExamplePage *example, UUIMenu *menu, const char *title,
                          const char *description, const char *controls);
bool ui_example_page_add(UIExamplePage *example, UUIElement *element);

void ui_example_renderer_init(UUIRenderer *renderer, UNeoGeoUIRenderer *backend,
                              UUITheme *theme);
void ui_example_input_from_controller(UUIInput *ui_input,
                                      const UInputController *controller);

#endif
