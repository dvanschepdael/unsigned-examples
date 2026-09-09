#include "versus_hud.h"

#include <stdio.h>
#include <ngdevkit/bios-calls.h>
#include <ngdevkit/ng-fix.h>

void versus_hud_clear(void) {
    bios_fix_clear();
}

static void hud_bar(char out[13], s16 health) {
    u8 filled = health > 0 ? (u8)(health / 10) : 0u;
    if (filled > 10u) filled = 10u;
    out[0] = '[';
    for (u8 i = 0u; i < 10u; ++i) out[i + 1u] = i < filled ? '#' : '-';
    out[11] = ']';
    out[12] = '\0';
}

void versus_hud_render(const VersusMatch *match) {
    char p1[13];
    char p2[13];
    char timer[8];
    if (match == NULL) return;

    hud_bar(p1, match->fighters[0].health);
    hud_bar(p2, match->fighters[1].health);
    (void)snprintf(timer, sizeof(timer), "%02u", (unsigned)(match->round_frames_remaining / match->refresh_rate));

    ng_text(1, 2, 0, "P1");
    ng_text(4, 2, 0, p1);
    ng_text(23, 2, 0, p2);
    ng_text(36, 2, 0, "P2");
    ng_center_text(4, 0, timer);

    if (match->phase == VERSUS_ROUND_INTRO) ng_center_text(10, 0, "ROUND - GET READY");
    else if (match->phase == VERSUS_ROUND_FIGHT) ng_center_text(10, 0, "FIGHT");
    else if (match->phase == VERSUS_ROUND_OUTRO) ng_center_text(10, 0, match->winner == 0xffu ? "DRAW" : match->winner == 0u ? "PLAYER 1 WINS" : "PLAYER 2 WINS");
    else if (match->phase == VERSUS_MATCH_OVER) ng_center_text(10, 0, match->rounds_won[0] > match->rounds_won[1] ? "PLAYER 1 MATCH WIN" : "PLAYER 2 MATCH WIN");

    ng_center_text(28, 0, "A LIGHT  B HEAVY  QCF+A SPECIAL");
}
