#ifndef VERSUS_GAME_H
#define VERSUS_GAME_H

#include "game/game.h"
#include "system/neogeo/runtime.h"

#include "versus_config.h"
#include "versus_match.h"

typedef struct VersusGame {
    /* Unsigned owns input, level, actor pools, collision, renderer and viewport. */
    UGameInstance runtime;

    /* Fixed memory required by UGameInstance. */
    UPoolInstance player_storage[VERSUS_PLAYER_COUNT];
    UActor *actor_storage[VERSUS_ACTOR_CAPACITY];
    const UCollisionBox *collision_layer_storage[VERSUS_COLLISION_LAYER_CAPACITY];
    const UCollisionBox *collision_query_storage[VERSUS_COLLISION_QUERY_CAPACITY];

    /* Game-specific state. The Neo Geo phase is cached because tick() has no phase argument. */
    VersusMatch match;
    UNeoGeoPhase phase;
} VersusGame;

bool versus_game_initialize(void *context);
void versus_game_shutdown(void *context);
void versus_game_start(void *context);
void versus_game_tick(void *context);
void versus_game_render(void *context);
void versus_game_enter_phase(void *context, UNeoGeoPhase phase);
void versus_game_render_phase(void *context, UNeoGeoPhase phase);

#endif
