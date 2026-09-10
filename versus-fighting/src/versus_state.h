#ifndef VERSUS_STATE_H
#define VERSUS_STATE_H

#include "core/state/state_graph.h"

/*
 * Adapter for the two simple enum-like state machines used by this POC.
 *
 * UStateGraph is still the real Unsigned state machine. This helper only builds
 * a flat event-driven graph where event N enters state N, then exposes the
 * current enum index. It keeps fighter and match code focused on their rules.
 */
typedef struct VersusStateMachine {
    UStateGraph graph;
    UStateGraphNode root;
    UStateGraphNodeContainer children;
    UStateGraphTransitionContainer events;
    const UStateGraphNode *nodes;
    u8 state_count;
} VersusStateMachine;

bool versus_state_machine_init(
    VersusStateMachine *machine,
    UStateGraphNode *nodes,
    UStateGraphTransition *transitions,
    u8 state_count,
    u8 initial_state,
    UStateGraphCallback on_enter,
    void *context
);

void versus_state_machine_set(VersusStateMachine *machine, u8 state);
u8 versus_state_machine_current_index(const VersusStateMachine *machine);

#endif
