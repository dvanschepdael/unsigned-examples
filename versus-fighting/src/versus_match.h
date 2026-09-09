#ifndef VERSUS_MATCH_H
#define VERSUS_MATCH_H

#include "display/viewport/viewport.h"
#include "input/input.h"

#include "versus_fighter.h"

typedef enum VersusRoundPhase {
    VERSUS_ROUND_INTRO = 0,
    VERSUS_ROUND_FIGHT,
    VERSUS_ROUND_OUTRO,
    VERSUS_MATCH_OVER,
} VersusRoundPhase;

typedef struct VersusMatch {
    VersusFighter fighters[2];
    VersusRoundPhase phase;
    u16 phase_frames;
    u16 round_frames_remaining;
    u16 refresh_rate;
    u8 rounds_won[2];
    u8 round_number;
    u8 hitstop_frames;
    u8 winner;
} VersusMatch;

bool versus_match_init(VersusMatch *match, u16 refresh_rate);
void versus_match_start(VersusMatch *match);
void versus_match_tick(VersusMatch *match, const UInputManager *input);
void versus_match_render(VersusMatch *match, const UViewport *viewport);
bool versus_match_finished(const VersusMatch *match);

#endif
