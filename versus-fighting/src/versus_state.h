#ifndef VERSUS_STATE_H
#define VERSUS_STATE_H

#include "core/state/state_graph.h"

/* Largest enum-style graph in this POC is the eight-state fighter graph. */
enum { VERSUS_STATE_MACHINE_CAPACITY = 8 };

/*
 * Fixed-storage adapter for the simple enum-like state machines used by this POC.
 *
 * UStateGraph is still the real Unsigned state machine. This helper only owns the
 * repetitive node/transition storage and builds a flat graph where event N enters
 * state N. There is no allocation and no second state variable to keep in sync.
 */
typedef struct VersusStateMachine {
    UStateGraph graph;
    UStateGraphNode root;
    UStateGraphNodeContainer children;
    UStateGraphTransitionContainer events;
    UStateGraphNode nodes[VERSUS_STATE_MACHINE_CAPACITY];
    UStateGraphTransition transitions[VERSUS_STATE_MACHINE_CAPACITY];
    u8 state_count;
} VersusStateMachine;

bool versus_state_machine_init(
    VersusStateMachine *machine,
    u8 state_count,
    u8 initial_state,
    UStateGraphCallback on_enter,
    void *context
);

void versus_state_machine_set(VersusStateMachine *machine, u8 state);
u8 versus_state_machine_current_index(const VersusStateMachine *machine);

#endif
