#ifndef VERSUS_FIGHTER_INTERNAL_H
#define VERSUS_FIGHTER_INTERNAL_H

#include "versus_fighter.h"

enum {
    VERSUS_ATTRIBUTE_HEALTH = 0,
    VERSUS_ATTRIBUTE_COUNT = 1,
};

bool versus_fighter_states_init(VersusFighter *fighter);
void versus_fighter_set_state(VersusFighter *fighter, VersusFighterState state);
void versus_fighter_play_state_animation(VersusFighter *fighter, VersusFighterState state);

void versus_fighter_input_reset(VersusFighter *fighter);
void versus_fighter_input_push(VersusFighter *fighter, const UInputController *controller);
bool versus_fighter_input_has_qcf(const VersusFighter *fighter);

#endif
