#ifndef UI_EXAMPLE_LEVELS_H
#define UI_EXAMPLE_LEVELS_H
#include <stddef.h>

#include "input/input.h"

typedef struct UIExampleApp UIExampleApp;

typedef struct UIExampleLevel {
    const char *name;
    bool (*enter)(UIExampleApp *app);
    void (*leave)(UIExampleApp *app);
    void (*tick)(UIExampleApp *app, const UInputController *controller);
} UIExampleLevel;

u8 ui_example_level_count(void);
const UIExampleLevel *ui_example_level_at(u8 index);

#endif
