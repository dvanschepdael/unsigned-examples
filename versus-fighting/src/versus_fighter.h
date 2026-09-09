#ifndef VERSUS_FIGHTER_H
#define VERSUS_FIGHTER_H

#include "core/types.h"
#include "display/sprite/sprite.h"
#include "input/input.h"
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
    USprite sprite;
    Vec2 position;
    Vec2 velocity;
    VersusInputBuffer input_buffer;
    VersusFighterState state;
    VersusAttackKind attack;
    s16 health;
    u8 controller_index;
    u8 facing_right;
    u8 hitstun_frames;
    u8 attack_connected;
} VersusFighter;

bool versus_fighter_init(VersusFighter *fighter, const USpriteDefinition *sprite, u16 first_sprite, u8 controller_index, s16 x, s16 y, bool facing_right);
void versus_fighter_reset(VersusFighter *fighter, s16 x, s16 y, bool facing_right);
void versus_fighter_tick(VersusFighter *fighter, const UInputController *controller, bool controls_enabled);
void versus_fighter_face_opponent(VersusFighter *fighter, const VersusFighter *opponent);
void versus_fighter_apply_hit(VersusFighter *fighter, s16 damage, u8 hitstun, s16 push_x);
void versus_fighter_apply_block(VersusFighter *fighter, u8 blockstun, s16 push_x);
bool versus_fighter_is_blocking(const VersusFighter *fighter, const UInputController *controller);
const UCollisionBox *versus_fighter_hitbox(VersusFighter *fighter);
const UCollisionBox *versus_fighter_hurtbox(VersusFighter *fighter);

#endif
