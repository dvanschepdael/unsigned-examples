#ifndef VERSUS_CONTENT_H
#define VERSUS_CONTENT_H

#include "display/sprite/sprite.h"

enum {
    VERSUS_SPRITE_WIDTH_TILES = 4,
    VERSUS_SPRITE_HEIGHT_TILES = 4,
    VERSUS_TILES_PER_FRAME = 16,
};

typedef enum VersusAnimation {
    VERSUS_ANIM_IDLE = 0,
    VERSUS_ANIM_WALK,
    VERSUS_ANIM_CROUCH,
    VERSUS_ANIM_JUMP,
    VERSUS_ANIM_LIGHT,
    VERSUS_ANIM_HEAVY,
    VERSUS_ANIM_SPECIAL,
    VERSUS_ANIM_BLOCK,
    VERSUS_ANIM_HIT,
    VERSUS_ANIM_KO,
    VERSUS_ANIM_COUNT,
} VersusAnimation;

typedef enum VersusAttackKind {
    VERSUS_ATTACK_NONE = 0,
    VERSUS_ATTACK_LIGHT,
    VERSUS_ATTACK_HEAVY,
    VERSUS_ATTACK_SPECIAL,
    VERSUS_ATTACK_COUNT,
} VersusAttackKind;

/* Immutable move tuning. Match logic reads this instead of hard-coding values. */
typedef struct VersusAttackDefinition {
    VersusAnimation animation;
    s16 damage;
    s16 pushback;
    u8 hitstun_frames;
    u8 blockstun_frames;
    u8 hitstop_frames;
    u8 block_hitstop_frames;
} VersusAttackDefinition;

extern const USpriteDefinition VERSUS_FIGHTER_P1_SPRITE;
extern const USpriteDefinition VERSUS_FIGHTER_P2_SPRITE;
extern const u16 VERSUS_P1_PALETTE[16];
extern const u16 VERSUS_P2_PALETTE[16];

const VersusAttackDefinition *versus_content_attack(VersusAttackKind attack);

#endif
