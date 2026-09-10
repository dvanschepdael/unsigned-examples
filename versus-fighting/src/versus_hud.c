#include "versus_hud.h"

#include <stdio.h>
#include <ngdevkit/bios-calls.h>
#include <ngdevkit/ng-fix.h>

void versus_hud_clear(void) {
    bios_fix_clear();
}

static void build_health_bar(char out[13], s16 health) {
    u8 filled = health > 0 ? (u8)(health / 10) : 0u;
    if (filled > 10u) filled = 10u;

    out[0] = '[';
    for (u8 i = 0u; i < 10u; ++i) {
        out[i + 1u] = i < filled ? '#' : '-';
    }
    out[11] = ']';
    out[12] = '\0';
}

static const char *round_message(const VersusMatch *match) {
    switch (versus_match_phase(match)) {
        case VERSUS_ROUND_INTRO:
            return "ROUND - GET READY";
        case VERSUS_ROUND_FIGHT:
            return "FIGHT";
        case VERSUS_ROUND_OUTRO:
            if (match->winner == 0xffu) return "DRAW";
            return match->winner == 0u ? "PLAYER 1 WINS" : "PLAYER 2 WINS";
        case VERSUS_MATCH_OVER:
            return match->rounds_won[0] > match->rounds_won[1] ? "PLAYER 1 MATCH WIN" : "PLAYER 2 MATCH WIN";
        default:
            return "";
    }
}

void versus_hud_render(const VersusMatch *match) {
    char p1[13];
    char p2[13];
    char timer[8];
    if (match == NULL || match->refresh_rate == 0u) return;

    build_health_bar(p1, versus_fighter_health(&match->fighters[0]));
    build_health_bar(p2, versus_fighter_health(&match->fighters[1]));
    (void)snprintf(timer, sizeof(timer), "%02u", (unsigned)(match->round_frames_remaining / match->refresh_rate));

    ng_text(1, 2, 0, "P1");
    ng_text(4, 2, 0, p1);
    ng_text(23, 2, 0, p2);
    ng_text(36, 2, 0, "P2");
    ng_center_text(4, 0, timer);
    ng_center_text(10, 0, round_message(match));
    ng_center_text(28, 0, "A LIGHT  B HEAVY  QCF+A SPECIAL");
}
