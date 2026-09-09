#include "versus_content.h"

#include "physics/collision.h"

#define TILE(n) ((u16)((n) * VERSUS_TILES_PER_FRAME))

static const UCollisionBox hurt_stand = { .w = 28, .h = 54, .offset_x = -14, .offset_y = -54 };
static const UCollisionBox hurt_crouch = { .w = 30, .h = 36, .offset_x = -15, .offset_y = -36 };
static const UCollisionBox hurt_air = { .w = 28, .h = 46, .offset_x = -14, .offset_y = -48 };
static const UCollisionBox hit_light = { .w = 24, .h = 14, .offset_x = 12, .offset_y = -42 };
static const UCollisionBox hit_heavy = { .w = 34, .h = 18, .offset_x = 10, .offset_y = -44 };
static const UCollisionBox hit_special = { .w = 42, .h = 22, .offset_x = 8, .offset_y = -46 };

static const UFrame idle_frames[] = {
    { .tile_offset = TILE(0), .duration = 6, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(1), .duration = 6, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(2), .duration = 6, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(3), .duration = 6, .hurtbox = &hurt_stand },
};
static const UFrame walk_frames[] = {
    { .tile_offset = TILE(4), .duration = 4, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(5), .duration = 4, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(6), .duration = 4, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(7), .duration = 4, .hurtbox = &hurt_stand },
};
static const UFrame crouch_frames[] = {
    { .tile_offset = TILE(8), .duration = 1, .hurtbox = &hurt_crouch },
};
static const UFrame jump_frames[] = {
    { .tile_offset = TILE(9), .duration = 5, .hurtbox = &hurt_air },
    { .tile_offset = TILE(10), .duration = 5, .hurtbox = &hurt_air },
};
static const UFrame light_frames[] = {
    { .tile_offset = TILE(11), .duration = 3, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(12), .duration = 2, .hitbox = &hit_light, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(13), .duration = 5, .hurtbox = &hurt_stand },
};
static const UFrame heavy_frames[] = {
    { .tile_offset = TILE(14), .duration = 5, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(15), .duration = 2, .hitbox = &hit_heavy, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(16), .duration = 2, .hitbox = &hit_heavy, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(17), .duration = 8, .hurtbox = &hurt_stand },
};
static const UFrame special_frames[] = {
    { .tile_offset = TILE(18), .duration = 4, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(19), .duration = 3, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(20), .duration = 3, .hitbox = &hit_special, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(21), .duration = 3, .hitbox = &hit_special, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(22), .duration = 9, .hurtbox = &hurt_stand },
};
static const UFrame block_frames[] = {
    { .tile_offset = TILE(23), .duration = 1, .hurtbox = &hurt_stand },
};
static const UFrame hit_frames[] = {
    { .tile_offset = TILE(24), .duration = 3, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(25), .duration = 3, .hurtbox = &hurt_stand },
};
static const UFrame ko_frames[] = {
    { .tile_offset = TILE(26), .duration = 6, .hurtbox = &hurt_stand },
    { .tile_offset = TILE(27), .duration = 6, .hurtbox = &hurt_crouch },
    { .tile_offset = TILE(28), .duration = 1, .hurtbox = &hurt_crouch },
};

static const UAnimation fighter_animations[] = {
    [VERSUS_ANIM_IDLE] = { .count = 4, .frames = idle_frames, .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1 },
    [VERSUS_ANIM_WALK] = { .count = 4, .frames = walk_frames, .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1 },
    [VERSUS_ANIM_CROUCH] = { .count = 1, .frames = crouch_frames, .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1 },
    [VERSUS_ANIM_JUMP] = { .count = 2, .frames = jump_frames, .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1 },
    [VERSUS_ANIM_LIGHT] = { .count = 3, .frames = light_frames, .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1 },
    [VERSUS_ANIM_HEAVY] = { .count = 4, .frames = heavy_frames, .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1 },
    [VERSUS_ANIM_SPECIAL] = { .count = 5, .frames = special_frames, .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1 },
    [VERSUS_ANIM_BLOCK] = { .count = 1, .frames = block_frames, .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1 },
    [VERSUS_ANIM_HIT] = { .count = 2, .frames = hit_frames, .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1 },
    [VERSUS_ANIM_KO] = { .count = 3, .frames = ko_frames, .hitbox_channel = U_COLLISION_CHANNEL_GAME_0, .hurtbox_channel = U_COLLISION_CHANNEL_GAME_1 },
};

static const UAnimationContainer fighter_animation_set = {
    .count = VERSUS_ANIM_COUNT,
    .instances = fighter_animations,
};

const USpriteDefinition VERSUS_FIGHTER_P1_SPRITE = {
    .animations = &fighter_animation_set,
    .first_tile = 0,
    .width_tiles = VERSUS_SPRITE_WIDTH_TILES,
    .height_tiles = VERSUS_SPRITE_HEIGHT_TILES,
    .palette = 1,
};
const USpriteDefinition VERSUS_FIGHTER_P2_SPRITE = {
    .animations = &fighter_animation_set,
    .first_tile = (u16)(29u * VERSUS_TILES_PER_FRAME),
    .width_tiles = VERSUS_SPRITE_WIDTH_TILES,
    .height_tiles = VERSUS_SPRITE_HEIGHT_TILES,
    .palette = 2,
};

const u16 VERSUS_P1_PALETTE[16] = {
    0x8000, 0x0fff, 0x0ccc, 0x0888, 0x0444, 0x0000, 0x0f80, 0x0c40,
    0x0820, 0x00bf, 0x007a, 0x0046, 0x0ff0, 0x0aa0, 0x0660, 0x0220,
};
const u16 VERSUS_P2_PALETTE[16] = {
    0x8000, 0x0fff, 0x0ccc, 0x0888, 0x0444, 0x0000, 0x00ff, 0x008c,
    0x0048, 0x0f08, 0x0a05, 0x0603, 0x0ff0, 0x0aa0, 0x0660, 0x0220,
};
