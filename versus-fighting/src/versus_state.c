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

    *machine = (VersusStateMachine){ 0 };
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
    if (machine == NULL || machine->events.instances == NULL || state >= machine->events.count) {
        return;
    }
    if (machine->graph.current == machine->events.instances[state].target) {
        return;
    }
    unsigned_state_graph_send_event(&machine->graph, (UEvent)state);
}

const UStateGraphNode *versus_state_machine_current(const VersusStateMachine *machine) {
    return machine != NULL ? machine->graph.current : NULL;
}

void versus_state_machine_tick(VersusStateMachine *machine) {
    if (machine != NULL) {
        unsigned_state_graph_tick(&machine->graph);
    }
}
