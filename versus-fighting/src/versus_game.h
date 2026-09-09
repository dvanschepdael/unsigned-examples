#ifndef VERSUS_GAME_H
#define VERSUS_GAME_H

#include "display/camera/camera.h"
#include "display/viewport/viewport.h"
#include "input/input.h"
#include "system/neogeo/runtime.h"

#include "versus_match.h"

typedef struct VersusGame {
    UInputManager input;
    UCamera camera;
    UViewport viewport;
    VersusMatch match;
    UNeoGeoPhase phase;
    bool match_started;
} VersusGame;

bool versus_game_initialize(void *context);
void versus_game_start(void *context);
void versus_game_tick(void *context);
void versus_game_render(void *context);
void versus_game_enter_phase(void *context, UNeoGeoPhase phase);
void versus_game_render_phase(void *context, UNeoGeoPhase phase);

#endif
