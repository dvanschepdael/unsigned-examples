#ifndef VERSUS_MATCH_H
#define VERSUS_MATCH_H

#include "input/input.h"

#include "versus_fighter.h"
#include "versus_state.h"

typedef enum VersusRoundPhase {
    VERSUS_ROUND_INTRO = 0,
    VERSUS_ROUND_FIGHT,
    VERSUS_ROUND_OUTRO,
    VERSUS_MATCH_OVER,
    VERSUS_ROUND_PHASE_COUNT,
} VersusRoundPhase;

typedef struct VersusMatch {
    VersusFighter fighters[VERSUS_PLAYER_COUNT];

    /* Round flow uses the same small UStateGraph adapter as fighters. */
    VersusStateMachine states;
    UStateGraphNode state_nodes[VERSUS_ROUND_PHASE_COUNT];
    UStateGraphTransition state_transitions[VERSUS_ROUND_PHASE_COUNT];

    const UInputManager *frame_input;
    u16 phase_frames;
    u16 round_frames_remaining;
    u16 refresh_rate;
    u8 rounds_won[VERSUS_PLAYER_COUNT];
    u8 round_number;
    u8 hitstop_frames;
    u8 winner;
} VersusMatch;

bool versus_match_init(VersusMatch *match, u16 refresh_rate);
void versus_match_start(VersusMatch *match);
void versus_match_update(VersusMatch *match, const UInputManager *input);

VersusRoundPhase versus_match_phase(const VersusMatch *match);
bool versus_match_finished(const VersusMatch *match);

/* Called by the arena's level callback when Unsigned reports an attacker/target pair. */
void versus_match_resolve_hit(VersusMatch *match, UActor *attacker, UActor *target);

#endif
