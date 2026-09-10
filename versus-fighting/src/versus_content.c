#include "versus_content.h"

#include "physics/collision.h"

#define TILE(n) ((u16)((n) * VERSUS_TILES_PER_FRAME))

static const UCollisionBox HURT_STAND = { .w = 28, .h = 54, .offset_x = -14, .offset_y = -54 };
static const UCollisionBox HURT_CROUCH = { .w = 30, .h = 36, .offset_x = -15, .offset_y = -36 };
static const UCollisionBox HURT_AIR = { .w = 28, .h = 46, .offset_x = -14, .offset_y = -48 };
static const UCollisionBox HIT_LIGHT = { .w = 24, .h = 14, .offset_x = 12, .offset_y = -42 };
static const UCollisionBox HIT_HEAVY = { .w = 34, .h = 18, .offset_x = 10, .offset_y = -44 };
static const UCollisionBox HIT_SPECIAL = { .w = 42, .h = 22, .offset_x = 8, .offset_y = -46 };

static const UFrame IDLE_FRAMES[] = {
    { .tile_offset = TILE(0), .duration = 6, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(1), .duration = 6, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(2), .duration = 6, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(3), .duration = 6, .hurtbox = &HURT_STAND },
};
static const UFrame WALK_FRAMES[] = {
    { .tile_offset = TILE(4), .duration = 4, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(5), .duration = 4, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(6), .duration = 4, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(7), .duration = 4, .hurtbox = &HURT_STAND },
};
static const UFrame CROUCH_FRAMES[] = {
    { .tile_offset = TILE(8), .duration = 1, .hurtbox = &HURT_CROUCH },
};
static const UFrame JUMP_FRAMES[] = {
    { .tile_offset = TILE(9), .duration = 5, .hurtbox = &HURT_AIR },
    { .tile_offset = TILE(10), .duration = 5, .hurtbox = &HURT_AIR },
};
static const UFrame LIGHT_FRAMES[] = {
    { .tile_offset = TILE(11), .duration = 3, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(12), .duration = 2, .hitbox = &HIT_LIGHT, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(13), .duration = 5, .hurtbox = &HURT_STAND },
};
static const UFrame HEAVY_FRAMES[] = {
    { .tile_offset = TILE(14), .duration = 5, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(15), .duration = 2, .hitbox = &HIT_HEAVY, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(16), .duration = 2, .hitbox = &HIT_HEAVY, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(17), .duration = 8, .hurtbox = &HURT_STAND },
};
static const UFrame SPECIAL_FRAMES[] = {
    { .tile_offset = TILE(18), .duration = 4, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(19), .duration = 3, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(20), .duration = 3, .hitbox = &HIT_SPECIAL, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(21), .duration = 3, .hitbox = &HIT_SPECIAL, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(22), .duration = 9, .hurtbox = &HURT_STAND },
};
static const UFrame BLOCK_FRAMES[] = {
    { .tile_offset = TILE(23), .duration = 1, .hurtbox = &HURT_STAND },
};
static const UFrame HIT_FRAMES[] = {
    { .tile_offset = TILE(24), .duration = 3, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(25), .duration = 3, .hurtbox = &HURT_STAND },
};
static const UFrame KO_FRAMES[] = {
    { .tile_offset = TILE(26), .duration = 6, .hurtbox = &HURT_STAND },
    { .tile_offset = TILE(27), .duration = 6, .hurtbox = &HURT_CROUCH },
    { .tile_offset = TILE(28), .duration = 1, .hurtbox = &HURT_CROUCH },
};

#define ANIMATION(frame_array) { \
    .count = ARRAY_COUNT_U8(frame_array), \
    .frames = frame_array, \
    .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, \
    .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1, \
}

static const UAnimation FIGHTER_ANIMATIONS[] = {
    [VERSUS_ANIM_IDLE] = ANIMATION(IDLE_FRAMES),
    [VERSUS_ANIM_WALK] = ANIMATION(WALK_FRAMES),
    [VERSUS_ANIM_CROUCH] = ANIMATION(CROUCH_FRAMES),
    [VERSUS_ANIM_JUMP] = ANIMATION(JUMP_FRAMES),
    [VERSUS_ANIM_LIGHT] = ANIMATION(LIGHT_FRAMES),
    [VERSUS_ANIM_HEAVY] = ANIMATION(HEAVY_FRAMES),
    [VERSUS_ANIM_SPECIAL] = ANIMATION(SPECIAL_FRAMES),
    [VERSUS_ANIM_BLOCK] = ANIMATION(BLOCK_FRAMES),
    [VERSUS_ANIM_HIT] = ANIMATION(HIT_FRAMES),
    [VERSUS_ANIM_KO] = ANIMATION(KO_FRAMES),
};

static const UAnimationContainer FIGHTER_ANIMATION_SET = {
    .count = ARRAY_COUNT_U8(FIGHTER_ANIMATIONS),
    .instances = FIGHTER_ANIMATIONS,
};

const USpriteDefinition VERSUS_FIGHTER_P1_SPRITE = {
    .animations = &FIGHTER_ANIMATION_SET,
    .first_tile = 0,
    .width_tiles = VERSUS_SPRITE_WIDTH_TILES,
    .height_tiles = VERSUS_SPRITE_HEIGHT_TILES,
    .palette = 1,
};

const USpriteDefinition VERSUS_FIGHTER_P2_SPRITE = {
    .animations = &FIGHTER_ANIMATION_SET,
    .first_tile = (u16)(29u * VERSUS_TILES_PER_FRAME),
    .width_tiles = VERSUS_SPRITE_WIDTH_TILES,
    .height_tiles = VERSUS_SPRITE_HEIGHT_TILES,
    .palette = 2,
};

static const VersusAttackDefinition ATTACKS[VERSUS_ATTACK_COUNT] = {
    [VERSUS_ATTACK_NONE] = { 0 },
    [VERSUS_ATTACK_LIGHT] = {
        .animation = VERSUS_ANIM_LIGHT,
        .damage = 8,
        .pushback = 2,
        .hitstun_frames = 10,
        .blockstun_frames = 7,
        .hitstop_frames = 6,
        .block_hitstop_frames = 4,
    },
    [VERSUS_ATTACK_HEAVY] = {
        .animation = VERSUS_ANIM_HEAVY,
        .damage = 14,
        .pushback = 2,
        .hitstun_frames = 16,
        .blockstun_frames = 7,
        .hitstop_frames = 6,
        .block_hitstop_frames = 4,
    },
    [VERSUS_ATTACK_SPECIAL] = {
        .animation = VERSUS_ANIM_SPECIAL,
        .damage = 20,
        .pushback = 2,
        .hitstun_frames = 22,
        .blockstun_frames = 7,
        .hitstop_frames = 6,
        .block_hitstop_frames = 4,
    },
};

const VersusAttackDefinition *versus_content_attack(VersusAttackKind attack) {
    return attack > VERSUS_ATTACK_NONE && attack < VERSUS_ATTACK_COUNT ? &ATTACKS[attack] : NULL;
}

const u16 VERSUS_P1_PALETTE[16] = {
    0x8000, 0x0fff, 0x0ccc, 0x0888, 0x0444, 0x0000, 0x0f80, 0x0c40,
    0x0820, 0x00bf, 0x007a, 0x0046, 0x0ff0, 0x0aa0, 0x0660, 0x0220,
};

const u16 VERSUS_P2_PALETTE[16] = {
    0x8000, 0x0fff, 0x0ccc, 0x0888, 0x0444, 0x0000, 0x00ff, 0x008c,
    0x0048, 0x0f08, 0x0a05, 0x0603, 0x0ff0, 0x0aa0, 0x0660, 0x0220,
};
