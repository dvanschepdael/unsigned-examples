#include "src/versus_game.h"

#include "system/neogeo/runtime.h"

static int run_game(bool mvs_title) {
    VersusGame game = { 0 };
    const UNeoGeoRuntimeDefinition runtime = {
        .context = &game,
        .input = &game.runtime.input,
        .initialize = versus_game_initialize,
        .start_game = versus_game_start,
        .tick = versus_game_tick,
        .render = versus_game_render,
        .enter_phase = versus_game_enter_phase,
        .render_phase = versus_game_render_phase,
    };
    return mvs_title ? unsigned_neo_geo_main_mvs(&runtime, 0u) : unsigned_neo_geo_main(&runtime, 0u);
}

int main(void) {
    return run_game(false);
}

int main_mvs_title(void) {
    return run_game(true);
}
