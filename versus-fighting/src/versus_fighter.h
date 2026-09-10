#ifndef VERSUS_FIGHTER_H
#define VERSUS_FIGHTER_H

#include "actor/player.h"
#include "core/state/state_graph.h"
#include "physics/collision.h"

#include "versus_content.h"

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

typedef enum VersusAttackKind {
    VERSUS_ATTACK_NONE = 0,
    VERSUS_ATTACK_LIGHT,
    VERSUS_ATTACK_HEAVY,
    VERSUS_ATTACK_SPECIAL,
} VersusAttackKind;

typedef struct VersusInputSample {
    u8 direction;
    UInputMask pressed;
} VersusInputSample;

typedef struct VersusInputBuffer {
    VersusInputSample samples[16];
    u8 head;
} VersusInputBuffer;

typedef struct VersusFighter {
    /* Unsigned owns the generic player -> character -> actor -> sprite layers. */
    UPlayer player;
    UCharacter character;

    /* Fighter-specific state is expressed through Unsigned's generic state graph. */
    UStateGraph state_graph;
    UStateGraphNode state_root;
    UStateGraphNode state_nodes[VERSUS_FIGHTER_STATE_COUNT];
    UStateGraphNodeContainer state_children;
    UStateGraphTransition state_events[VERSUS_FIGHTER_STATE_COUNT];
    UStateGraphTransitionContainer state_event_container;

    Vec2 velocity;
    VersusInputBuffer input_buffer;
    VersusFighterState state;
    VersusAttackKind attack;
    s16 health;
    u8 hitstun_frames;
    u8 attack_connected;
} VersusFighter;

bool versus_fighter_init(VersusFighter *fighter, const USpriteDefinition *sprite, u16 first_sprite, u8 controller_index, s16 x, s16 y, bool facing_right);
void versus_fighter_reset(VersusFighter *fighter, s16 x, s16 y, bool facing_right);
void versus_fighter_update(VersusFighter *fighter, const UInputController *controller, bool controls_enabled);
void versus_fighter_face_opponent(VersusFighter *fighter, const VersusFighter *opponent);
void versus_fighter_apply_hit(VersusFighter *fighter, s16 damage, u8 hitstun, s16 push_x);
void versus_fighter_apply_block(VersusFighter *fighter, u8 blockstun, s16 push_x);
bool versus_fighter_is_blocking(const VersusFighter *fighter, const UInputController *controller);
const UCollisionBox *versus_fighter_hitbox(VersusFighter *fighter);
const UCollisionBox *versus_fighter_hurtbox(VersusFighter *fighter);

static inline UActor *versus_fighter_actor(VersusFighter *fighter) {
    return fighter != NULL ? &fighter->character.actor : NULL;
}

static inline const UActor *versus_fighter_actor_const(const VersusFighter *fighter) {
    return fighter != NULL ? &fighter->character.actor : NULL;
}

#endif
