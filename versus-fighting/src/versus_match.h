#ifndef VERSUS_MATCH_H
#define VERSUS_MATCH_H

#include "core/state/state_graph.h"
#include "input/input.h"
#include "level/level_definition.h"
#include "level/level_runtime.h"

#include "versus_fighter.h"

typedef enum VersusRoundPhase {
    VERSUS_ROUND_INTRO = 0,
    VERSUS_ROUND_FIGHT,
    VERSUS_ROUND_OUTRO,
    VERSUS_MATCH_OVER,
    VERSUS_ROUND_PHASE_COUNT,
} VersusRoundPhase;

typedef struct VersusMatch {
    VersusFighter fighters[2];

    /* Match/round flow uses the same generic state graph primitive as fighters. */
    UStateGraph state_graph;
    UStateGraphNode state_root;
    UStateGraphNode state_nodes[VERSUS_ROUND_PHASE_COUNT];
    UStateGraphNodeContainer state_children;
    UStateGraphTransition state_events[VERSUS_ROUND_PHASE_COUNT];
    UStateGraphTransitionContainer state_event_container;

    const UInputManager *frame_input;
    VersusRoundPhase phase;
    u16 phase_frames;
    u16 round_frames_remaining;
    u16 refresh_rate;
    u8 rounds_won[2];
    u8 round_number;
    u8 hitstop_frames;
    u8 winner;
} VersusMatch;

extern const ULevelDefinition VERSUS_ARENA_LEVEL;

bool versus_match_init(VersusMatch *match, u16 refresh_rate);
void versus_match_start(VersusMatch *match);
void versus_match_update(VersusMatch *match, const UInputManager *input);
bool versus_match_finished(const VersusMatch *match);

#endif
