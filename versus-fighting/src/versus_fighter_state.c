#include "versus_fighter_internal.h"

static VersusFighterState fighter_state_from_node(const VersusFighter *fighter, const UStateGraphNode *node) {
    for (u8 i = 0u; i < VERSUS_FIGHTER_STATE_COUNT; ++i) {
        if (node == &fighter->state_nodes[i]) return (VersusFighterState)i;
    }
    return VERSUS_FIGHTER_IDLE;
}

static void fighter_play(VersusFighter *fighter, VersusAnimation animation, UAnimationPlayback playback) {
    USprite *sprite = &fighter->character.actor.sprite;
    if (sprite->animation_index != (u8)animation || sprite->state == U_SPRITE_COMPLETED) {
        (void)unsigned_sprite_play(sprite, (u8)animation, playback);
    }
}

void versus_fighter_play_state_animation(VersusFighter *fighter, VersusFighterState state) {
    if (fighter == NULL) return;

    switch (state) {
        case VERSUS_FIGHTER_IDLE: fighter_play(fighter, VERSUS_ANIM_IDLE, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_WALK: fighter_play(fighter, VERSUS_ANIM_WALK, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_CROUCH: fighter_play(fighter, VERSUS_ANIM_CROUCH, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_JUMP: fighter_play(fighter, VERSUS_ANIM_JUMP, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_BLOCK: fighter_play(fighter, VERSUS_ANIM_BLOCK, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_HITSTUN: fighter_play(fighter, VERSUS_ANIM_HIT, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_KO: fighter_play(fighter, VERSUS_ANIM_KO, U_SPRITE_PLAY_ONCE); break;
        case VERSUS_FIGHTER_ATTACK: {
            const VersusAttackDefinition *attack = versus_content_attack(fighter->attack);
            if (attack != NULL) fighter_play(fighter, attack->animation, U_SPRITE_PLAY_ONCE);
            break;
        }
        default: break;
    }
}

static void fighter_state_enter(UStateGraph *graph, void *context) {
    VersusFighter *fighter = context;
    if (fighter == NULL || graph == NULL) return;
    versus_fighter_play_state_animation(fighter, fighter_state_from_node(fighter, graph->current));
}

bool versus_fighter_states_init(VersusFighter *fighter) {
    if (fighter == NULL) return false;
    return versus_state_machine_init(&fighter->states, fighter->state_nodes, fighter->state_transitions,
        VERSUS_FIGHTER_STATE_COUNT, VERSUS_FIGHTER_IDLE, fighter_state_enter, fighter);
}

void versus_fighter_set_state(VersusFighter *fighter, VersusFighterState state) {
    if (fighter != NULL && state < VERSUS_FIGHTER_STATE_COUNT) {
        versus_state_machine_set(&fighter->states, (u8)state);
    }
}

VersusFighterState versus_fighter_state(const VersusFighter *fighter) {
    if (fighter == NULL) return VERSUS_FIGHTER_IDLE;
    return fighter_state_from_node(fighter, versus_state_machine_current(&fighter->states));
}
