#include "versus_fighter_internal.h"

enum {
    DIR_DOWN = 2,
    DIR_DOWN_FORWARD = 3,
    DIR_FORWARD = 6,
};

static u8 relative_direction(const VersusFighter *fighter, const UInputController *controller) {
    const bool up = (controller->state.down & U_INPUT_BUTTON_UP) != 0u;
    const bool down = (controller->state.down & U_INPUT_BUTTON_DOWN) != 0u;
    const bool left = (controller->state.down & U_INPUT_BUTTON_LEFT) != 0u;
    const bool right = (controller->state.down & U_INPUT_BUTTON_RIGHT) != 0u;
    const bool forward = fighter->character.facing_right ? right : left;
    const bool back = fighter->character.facing_right ? left : right;

    if (down && forward) return 3u;
    if (down && back) return 1u;
    if (up && forward) return 9u;
    if (up && back) return 7u;
    if (down) return 2u;
    if (up) return 8u;
    if (forward) return 6u;
    if (back) return 4u;
    return 5u;
}

void versus_fighter_input_reset(VersusFighter *fighter) {
    if (fighter != NULL) fighter->input_buffer = (VersusInputBuffer){ 0 };
}

void versus_fighter_input_push(VersusFighter *fighter, const UInputController *controller) {
    if (fighter == NULL || controller == NULL) return;

    fighter->input_buffer.head = (u8)((fighter->input_buffer.head + 1u) & (VERSUS_INPUT_BUFFER_CAPACITY - 1u));
    fighter->input_buffer.samples[fighter->input_buffer.head].direction = relative_direction(fighter, controller);
}

bool versus_fighter_input_has_qcf(const VersusFighter *fighter) {
    bool saw_forward = false;
    bool saw_down_forward = false;
    if (fighter == NULL) return false;

    for (u8 age = 0u; age < VERSUS_QCF_WINDOW; ++age) {
        const u8 index = (u8)((fighter->input_buffer.head - age) & (VERSUS_INPUT_BUFFER_CAPACITY - 1u));
        const u8 direction = fighter->input_buffer.samples[index].direction;

        if (!saw_forward && direction == DIR_FORWARD) saw_forward = true;
        else if (saw_forward && !saw_down_forward && direction == DIR_DOWN_FORWARD) saw_down_forward = true;
        else if (saw_down_forward && direction == DIR_DOWN) return true;
    }
    return false;
}
