#include "versus_game.h"

#include "display/sprite/palette.h"
#include "system/neogeo/video.h"
#include "versus_content.h"
#include "versus_hud.h"

#include <ngdevkit/bios-calls.h>
#include <ngdevkit/neogeo.h>
#include <ngdevkit/ng-fix.h>

static void versus_load_fix_palette(void) {
    /* Palette 0 is used by ng_text/ng_center_text and by the HUD. */
    MMAP_PALBANK1[0] = 0x8000;
    MMAP_PALBANK1[1] = 0x0fff;
    MMAP_PALBANK1[2] = 0x0555;
}

bool versus_game_initialize(void *context) {
    VersusGame *game = context;
    if (game == NULL || !unsigned_input_manager_init(&game->input, 2u)) return false;

    game->camera = (UCamera){ .x = 0, .y = 0 };
    unsigned_viewport_init(&game->viewport, 0, 0, 320, 224, &game->camera);

    versus_load_fix_palette();
    unsigned_sprite_palette_load(1u, VERSUS_P1_PALETTE);
    unsigned_sprite_palette_load(2u, VERSUS_P2_PALETTE);
    unsigned_sprite_palette_set_backdrop_color(0x0000);

    if (!versus_match_init(&game->match, unsigned_video_get_refresh_rate())) return false;

    game->match_started = false;
    return true;
}

void versus_game_start(void *context) {
    VersusGame *game = context;
    if (game == NULL) return;
    versus_match_start(&game->match);
    game->match_started = true;
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

    if (game->phase == U_NEO_GEO_PHASE_GAME && game->match_started) {
        versus_match_tick(&game->match, &game->input);
        if (versus_match_finished(&game->match)) unsigned_neo_geo_request_game_over();
    } else if (game->phase == U_NEO_GEO_PHASE_GAME_OVER && (game->input.players[0].state.pressed & U_INPUT_BUTTON_A) != 0u) {
        unsigned_neo_geo_end_session();
    }
}

void versus_game_render(void *context) {
    VersusGame *game = context;
    if (game == NULL || game->phase != U_NEO_GEO_PHASE_GAME || !game->match_started) return;
    versus_match_render(&game->match, &game->viewport);
    versus_hud_render(&game->match);
}

void versus_game_render_phase(void *context, UNeoGeoPhase phase) {
    (void)context;
    (void)phase;
}
