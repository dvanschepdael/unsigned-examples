#include "versus_arena.h"

#include "actor/player_pool.h"
#include "level/level_actor.h"

#include "versus_config.h"
#include "versus_content.h"
#include "versus_match.h"

static bool arena_load(ULevel *level, const ULevelDefinition *definition, void *context) {
    VersusMatch *match = context;
    UPoolInstanceContainer *players;
    (void)definition;

    if (level == NULL || match == NULL) return false;
    players = unsigned_level_player_pool(level);
    if (players == NULL) return false;

    if (!versus_fighter_init(
            &match->fighters[0],
            &VERSUS_FIGHTER_P1_SPRITE,
            VERSUS_PLAYER_1_FIRST_SPRITE,
            0u,
            (Vec2){ VERSUS_PLAYER_1_START_X, VERSUS_GROUND_Y },
            true)) {
        return false;
    }

    if (!versus_fighter_init(
            &match->fighters[1],
            &VERSUS_FIGHTER_P2_SPRITE,
            VERSUS_PLAYER_2_FIRST_SPRITE,
            1u,
            (Vec2){ VERSUS_PLAYER_2_START_X, VERSUS_GROUND_Y },
            false)) {
        return false;
    }

    if (unsigned_player_pool_reserve(players, &match->fighters[0].player, 0u) == NULL) return false;
    if (unsigned_player_pool_reserve(players, &match->fighters[1].player, 1u) == NULL) return false;
    return true;
}

static void arena_resolve_hits(ULevel *level, const ULevelDefinition *definition, void *context) {
    VersusMatch *match = context;
    (void)definition;

    if (level == NULL || match == NULL) return;
    for (u16 i = 0u; i < level->collision.hits.count; ++i) {
        UCollisionHit *hit = &level->collision.hits.instances[i];
        versus_match_resolve_hit(match, hit->attacker, hit->target);
    }
}

const ULevelDefinition VERSUS_ARENA_LEVEL = {
    .load = arena_load,
    .actor_order = U_ACTOR_ORDER_Y_X_STABLE,
    .resolve_hits = arena_resolve_hits,
    .backdrop_color = 0x0000,
};
