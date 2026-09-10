#ifndef VERSUS_GAME_H
#define VERSUS_GAME_H

#include "game/config.h"
#include "game/game.h"
#include "system/neogeo/runtime.h"

#include "versus_match.h"

enum {
    VERSUS_PLAYER_CAPACITY = 2,
    VERSUS_NPC_CAPACITY = 0,
    VERSUS_OBJECT_CAPACITY = 0,
    VERSUS_PROJECTILE_CAPACITY = 0,
    VERSUS_ACTOR_CAPACITY = U_GAME_ACTOR_CAPACITY(2, 0, 0, 0),
    VERSUS_COLLISION_LAYER_CAPACITY = U_GAME_COLLISION_LAYER_STORAGE_CAPACITY(2, 0, 0, 0),
    VERSUS_COLLISION_QUERY_CAPACITY = U_GAME_COLLISION_QUERY_STORAGE_CAPACITY(2, 0, 0, 0),
};

typedef struct VersusGame {
    /* Unsigned owns input, level, actor pools, collision, renderer and viewport. */
    UGameInstance runtime;

    /* Caller-owned fixed storage required by UGameInstance. */
    UPoolInstance player_storage[VERSUS_PLAYER_CAPACITY];
    UActor *actor_storage[VERSUS_ACTOR_CAPACITY];
    const UCollisionBox *collision_layer_storage[VERSUS_COLLISION_LAYER_CAPACITY];
    const UCollisionBox *collision_query_storage[VERSUS_COLLISION_QUERY_CAPACITY];

    /* Only genre-specific orchestration remains here. */
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
