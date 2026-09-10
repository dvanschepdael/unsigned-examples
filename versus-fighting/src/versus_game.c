#include "versus_game.h"

#include "display/sprite/palette.h"
#include "system/neogeo/video.h"

#include "versus_arena.h"
#include "versus_content.h"
#include "versus_hud.h"

#include <ngdevkit/bios-calls.h>
#include <ngdevkit/neogeo.h>
#include <ngdevkit/ng-fix.h>

static void load_fix_palette(void) {
    MMAP_PALBANK1[0] = 0x8000;
    MMAP_PALBANK1[1] = 0x0fff;
    MMAP_PALBANK1[2] = 0x0555;
}

static UGameInstanceConfig build_runtime_config(VersusGame *game, u8 refresh_rate) {
    return (UGameInstanceConfig){
        .player_capacity = VERSUS_PLAYER_COUNT,
        .npc_capacity = VERSUS_NPC_CAPACITY,
        .object_capacity = VERSUS_OBJECT_CAPACITY,
        .projectile_capacity = VERSUS_PROJECTILE_CAPACITY,
        .refresh_rate = refresh_rate,
        .tlss = {
            .ai = U_TLSS_SCALE_1,
            .collision = U_TLSS_SCALE_1,
        },
        .storage = {
            .players = game->player_storage,
            .actors = game->actor_storage,
            .actor_capacity = VERSUS_ACTOR_CAPACITY,
            .collision_layer_boxes = game->collision_layer_storage,
            .collision_layer_box_capacity = VERSUS_COLLISION_LAYER_CAPACITY,
            .collision_query_hits = game->collision_query_storage,
            .collision_query_hit_capacity = VERSUS_COLLISION_QUERY_CAPACITY,
        },
        .initial_level = &VERSUS_ARENA_LEVEL,
        .level_context = &game->match,
    };
}

bool versus_game_initialize(void *context) {
    VersusGame *game = context;
    if (game == NULL) return false;

    const u16 refresh_rate = unsigned_video_get_refresh_rate();
    if (refresh_rate == 0u || refresh_rate > UINT8_MAX) return false;
    if (!versus_match_init(&game->match, refresh_rate)) return false;

    const UGameInstanceConfig config = build_runtime_config(game, (u8)refresh_rate);
    if (!unsigned_game_instance_init(&game->runtime, &config)) return false;

    load_fix_palette();
    unsigned_sprite_palette_load(1u, VERSUS_P1_PALETTE);
    unsigned_sprite_palette_load(2u, VERSUS_P2_PALETTE);
    return true;
}

void versus_game_shutdown(void *context) {
    VersusGame *game = context;
    if (game != NULL) {
        unsigned_game_instance_destroy(&game->runtime);
    }
}

void versus_game_start(void *context) {
    VersusGame *game = context;
    if (game == NULL) return;

    versus_match_start(&game->match);
    versus_hud_clear();
}

void versus_game_enter_phase(void *context, UNeoGeoPhase phase) {
    VersusGame *game = context;
    if (game == NULL) return;

    game->phase = phase;
    bios_fix_clear();

    if (phase == U_NEO_GEO_PHASE_ATTRACT || phase == U_NEO_GEO_PHASE_TITLE) {
        ng_center_text(8, 0, "UNSIGNED VERSUS FIGHTING POC");
        ng_center_text(13, 0, "2 PLAYER LOCAL VERSUS");
        ng_center_text(18, 0, "INSERT COIN / PRESS START");
    } else if (phase == U_NEO_GEO_PHASE_GAME_OVER) {
        ng_center_text(13, 0, "MATCH OVER");
        ng_center_text(17, 0, "A: RETURN TO TITLE");
    }
}

void versus_game_tick(void *context) {
    VersusGame *game = context;
    if (game == NULL) return;

    UInputManager *input = unsigned_game_instance_input(&game->runtime);
    if (input == NULL) return;

    if (game->phase == U_NEO_GEO_PHASE_GAME) {
        /* Hitstop freezes both game-specific logic and Unsigned's actor/animation world tick. */
        const bool world_was_paused = versus_match_world_paused(&game->match);
        versus_match_update(&game->match, input);
        if (!world_was_paused) {
            unsigned_game_instance_tick(&game->runtime);
        }

        if (versus_match_finished(&game->match)) {
            unsigned_neo_geo_request_game_over();
        }
    } else if (game->phase == U_NEO_GEO_PHASE_GAME_OVER &&
               (input->players[0].state.pressed & U_INPUT_BUTTON_A) != 0u) {
        unsigned_neo_geo_end_session();
    }
}

void versus_game_render(void *context) {
    VersusGame *game = context;
    if (game == NULL || game->phase != U_NEO_GEO_PHASE_GAME) return;

    unsigned_game_instance_render(&game->runtime);
    versus_hud_render(&game->match);
}
