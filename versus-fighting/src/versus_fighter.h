#ifndef VERSUS_FIGHTER_H
#define VERSUS_FIGHTER_H

#include "actor/player.h"
#include "gameplay/attribute.h"

#include "versus_config.h"
#include "versus_content.h"
#include "versus_state.h"

typedef enum VersusFighterState {
    VERSUS_FIGHTER_IDLE = 0,
    VERSUS_FIGHTER_WALK,
    VERSUS_FIGHTER_CROUCH,
    VERSUS_FIGHTER_JUMP,
    VERSUS_FIGHTER_ATTACK,
    VERSUS_FIGHTER_BLOCK,
    VERSUS_FIGHTER_HITSTUN,
    VERSUS_FIGHTER_KO,
    VERSUS_FIGHTER_STATE_COUNT,
} VersusFighterState;

typedef struct VersusInputSample {
    u8 direction;
    UInputMask pressed;
} VersusInputSample;

typedef struct VersusInputBuffer {
    VersusInputSample samples[VERSUS_INPUT_BUFFER_CAPACITY];
    u8 head;
} VersusInputBuffer;

typedef struct VersusFighter {
    /* Generic Unsigned runtime. */
    UPlayer player;
    UCharacter character;

    /* One attribute is enough for this POC: health. */
    UGameplayAttribute attributes[1];

    /* Fighter state is backed by Unsigned's UStateGraph. */
    VersusStateMachine states;
    UStateGraphNode state_nodes[VERSUS_FIGHTER_STATE_COUNT];
    UStateGraphTransition state_transitions[VERSUS_FIGHTER_STATE_COUNT];

    /* Only fighting-game-specific runtime remains below. */
    VersusInputBuffer input_buffer;
    Vec2 velocity;
    VersusAttackKind attack;
    u8 stun_frames;
    bool attack_connected;
} VersusFighter;

bool versus_fighter_init(
    VersusFighter *fighter,
    const USpriteDefinition *sprite_definition,
    u16 first_sprite,
    u8 controller_index,
    Vec2 position,
    bool facing_right
);

void versus_fighter_reset(VersusFighter *fighter, Vec2 position, bool facing_right);
void versus_fighter_update(VersusFighter *fighter, const UInputController *controller, bool controls_enabled);
void versus_fighter_face_opponent(VersusFighter *fighter, const VersusFighter *opponent);

void versus_fighter_apply_hit(VersusFighter *fighter, const VersusAttackDefinition *attack, s16 direction);
void versus_fighter_apply_block(VersusFighter *fighter, const VersusAttackDefinition *attack, s16 direction);

VersusFighterState versus_fighter_state(const VersusFighter *fighter);
s16 versus_fighter_health(const VersusFighter *fighter);
bool versus_fighter_is_blocking(const VersusFighter *fighter, const UInputController *controller);

static inline UActor *versus_fighter_actor(VersusFighter *fighter) {
    return fighter != NULL ? &fighter->character.actor : NULL;
}

static inline const UActor *versus_fighter_actor_const(const VersusFighter *fighter) {
    return fighter != NULL ? &fighter->character.actor : NULL;
}

#endif
