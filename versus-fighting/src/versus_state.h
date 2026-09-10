#ifndef VERSUS_STATE_H
#define VERSUS_STATE_H

#include "core/state/state_graph.h"

/*
 * Small adapter used by this example for enum-like state machines.
 *
 * Unsigned's UStateGraph remains the real state machine. This wrapper only removes
 * the repetitive boilerplate required to build a flat event-driven graph where
 * event N enters state N. Fighter and match flow can therefore share the same setup.
 */
typedef struct VersusStateMachine {
    UStateGraph graph;
    UStateGraphNode root;
    UStateGraphNodeContainer children;
    UStateGraphTransitionContainer events;
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
const UStateGraphNode *versus_state_machine_current(const VersusStateMachine *machine);
void versus_state_machine_tick(VersusStateMachine *machine);

#endif
