#include "versus_state.h"

bool versus_state_machine_init(
    VersusStateMachine *machine,
    UStateGraphNode *nodes,
    UStateGraphTransition *transitions,
    u8 state_count,
    u8 initial_state,
    UStateGraphCallback on_enter,
    void *context
) {
    if (machine == NULL || nodes == NULL || transitions == NULL || state_count == 0u || initial_state >= state_count) {
        return false;
    }

    *machine = (VersusStateMachine){
        .nodes = nodes,
        .state_count = state_count,
    };
    machine->children = (UStateGraphNodeContainer){
        .count = state_count,
        .capacity = state_count,
        .instances = nodes,
    };
    machine->events = (UStateGraphTransitionContainer){
        .count = state_count,
        .capacity = state_count,
        .instances = transitions,
    };
    machine->root.edges = &machine->children;
    machine->root.transitions[U_TRANSITION_ON_EVENT] = &machine->events;

    for (u8 i = 0u; i < state_count; ++i) {
        nodes[i] = (UStateGraphNode){
            .parent = &machine->root,
            .enter = on_enter,
        };
        transitions[i] = (UStateGraphTransition){
            .target = &nodes[i],
        };
    }

    return unsigned_state_graph_init(&machine->graph, &machine->root, &nodes[initial_state], context);
}

void versus_state_machine_set(VersusStateMachine *machine, u8 state) {
    if (machine == NULL || state >= machine->state_count) return;
    if (machine->graph.current == &machine->nodes[state]) return;
    unsigned_state_graph_send_event(&machine->graph, (UEvent)state);
}

u8 versus_state_machine_current_index(const VersusStateMachine *machine) {
    if (machine == NULL || machine->graph.current == NULL || machine->nodes == NULL) return 0u;

    for (u8 i = 0u; i < machine->state_count; ++i) {
        if (machine->graph.current == &machine->nodes[i]) return i;
    }
    return 0u;
}
