#include "versus_fighter.h"

#define VERSUS_GROUND_Y 184
#define VERSUS_STAGE_LEFT 24
#define VERSUS_STAGE_RIGHT 296
#define VERSUS_WALK_SPEED 2
#define VERSUS_JUMP_SPEED (-7)
#define VERSUS_GRAVITY 1

enum {
    DIR_NEUTRAL = 5,
    DIR_DOWN = 2,
    DIR_DOWN_FORWARD = 3,
    DIR_FORWARD = 6,
};

static s16 clamp_s16(s16 value, s16 minimum, s16 maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static u8 fighter_relative_direction(const VersusFighter *fighter, const UInputController *controller) {
    bool up = (controller->state.down & U_INPUT_BUTTON_UP) != 0u;
    bool down = (controller->state.down & U_INPUT_BUTTON_DOWN) != 0u;
    bool left = (controller->state.down & U_INPUT_BUTTON_LEFT) != 0u;
    bool right = (controller->state.down & U_INPUT_BUTTON_RIGHT) != 0u;
    bool forward = fighter->facing_right ? right : left;
    bool back = fighter->facing_right ? left : right;

    if (down && forward) return 3;
    if (down && back) return 1;
    if (up && forward) return 9;
    if (up && back) return 7;
    if (down) return 2;
    if (up) return 8;
    if (forward) return 6;
    if (back) return 4;
    return 5;
}

static void fighter_buffer_push(VersusFighter *fighter, const UInputController *controller) {
    fighter->input_buffer.head = (u8)((fighter->input_buffer.head + 1u) & 15u);
    fighter->input_buffer.samples[fighter->input_buffer.head] = (VersusInputSample){
        .direction = fighter_relative_direction(fighter, controller),
        .pressed = controller->state.pressed,
    };
}

static bool fighter_has_qcf(const VersusFighter *fighter) {
    bool saw_forward = false;
    bool saw_down_forward = false;

    for (u8 age = 0u; age < 10u; ++age) {
        u8 index = (u8)((fighter->input_buffer.head - age) & 15u);
        u8 direction = fighter->input_buffer.samples[index].direction;
        if (!saw_forward && direction == DIR_FORWARD) {
            saw_forward = true;
        } else if (saw_forward && !saw_down_forward && direction == DIR_DOWN_FORWARD) {
            saw_down_forward = true;
        } else if (saw_down_forward && direction == DIR_DOWN) {
            return true;
        }
    }
    return false;
}

static void fighter_play(VersusFighter *fighter, VersusAnimation animation, UAnimationPlayback playback) {
    if (fighter->sprite.animation_index != animation || fighter->sprite.state == U_SPRITE_COMPLETED) {
        (void)unsigned_sprite_play(&fighter->sprite, (u8)animation, playback);
    }
}

static void fighter_start_attack(VersusFighter *fighter, VersusAttackKind attack) {
    fighter->state = VERSUS_FIGHTER_ATTACK;
    fighter->attack = attack;
    fighter->attack_connected = 0u;
    switch (attack) {
        case VERSUS_ATTACK_LIGHT:
            fighter_play(fighter, VERSUS_ANIM_LIGHT, U_SPRITE_PLAY_ONCE);
            break;
        case VERSUS_ATTACK_HEAVY:
            fighter_play(fighter, VERSUS_ANIM_HEAVY, U_SPRITE_PLAY_ONCE);
            break;
        case VERSUS_ATTACK_SPECIAL:
            fighter_play(fighter, VERSUS_ANIM_SPECIAL, U_SPRITE_PLAY_ONCE);
            break;
        case VERSUS_ATTACK_NONE:
        default:
            break;
    }
}

bool versus_fighter_init(VersusFighter *fighter, const USpriteDefinition *sprite, u16 first_sprite, u8 controller_index, s16 x, s16 y, bool facing_right) {
    if (fighter == NULL || sprite == NULL || !unsigned_sprite_init(&fighter->sprite, sprite, first_sprite)) {
        return false;
    }
    fighter->controller_index = controller_index;
    versus_fighter_reset(fighter, x, y, facing_right);
    return true;
}

void versus_fighter_reset(VersusFighter *fighter, s16 x, s16 y, bool facing_right) {
    if (fighter == NULL) return;
    fighter->position = (Vec2){ .x = x, .y = y };
    fighter->velocity = (Vec2){ 0 };
    fighter->input_buffer = (VersusInputBuffer){ 0 };
    fighter->state = VERSUS_FIGHTER_IDLE;
    fighter->attack = VERSUS_ATTACK_NONE;
    fighter->health = 100;
    fighter->facing_right = facing_right ? 1u : 0u;
    fighter->hitstun_frames = 0u;
    fighter->attack_connected = 0u;
    unsigned_sprite_set_flip_x(&fighter->sprite, !facing_right);
    (void)unsigned_sprite_play(&fighter->sprite, VERSUS_ANIM_IDLE, U_SPRITE_PLAY_LOOP);
}

void versus_fighter_face_opponent(VersusFighter *fighter, const VersusFighter *opponent) {
    if (fighter == NULL || opponent == NULL || fighter->state == VERSUS_FIGHTER_KO) return;
    fighter->facing_right = fighter->position.x <= opponent->position.x ? 1u : 0u;
    unsigned_sprite_set_flip_x(&fighter->sprite, fighter->facing_right == 0u);
}

bool versus_fighter_is_blocking(const VersusFighter *fighter, const UInputController *controller) {
    if (fighter == NULL || controller == NULL || fighter->state == VERSUS_FIGHTER_KO || fighter->state == VERSUS_FIGHTER_HITSTUN) return false;
    UInputMask back = fighter->facing_right ? U_INPUT_BUTTON_LEFT : U_INPUT_BUTTON_RIGHT;
    return (controller->state.down & back) != 0u;
}

void versus_fighter_tick(VersusFighter *fighter, const UInputController *controller, bool controls_enabled) {
    if (fighter == NULL || controller == NULL) return;

    fighter_buffer_push(fighter, controller);

    if (fighter->state == VERSUS_FIGHTER_KO) {
        fighter_play(fighter, VERSUS_ANIM_KO, U_SPRITE_PLAY_ONCE);
        unsigned_sprite_tick(&fighter->sprite, fighter);
        return;
    }

    if (fighter->hitstun_frames > 0u) {
        --fighter->hitstun_frames;
        bool blocking = fighter->state == VERSUS_FIGHTER_BLOCK;
        fighter->state = blocking ? VERSUS_FIGHTER_BLOCK : VERSUS_FIGHTER_HITSTUN;
        fighter_play(fighter, blocking ? VERSUS_ANIM_BLOCK : VERSUS_ANIM_HIT, U_SPRITE_PLAY_LOOP);
        fighter->position.x = clamp_s16((s16)(fighter->position.x + fighter->velocity.x), VERSUS_STAGE_LEFT, VERSUS_STAGE_RIGHT);
        fighter->velocity.x = 0;
        if (fighter->hitstun_frames == 0u) fighter->state = VERSUS_FIGHTER_IDLE;
        unsigned_sprite_tick(&fighter->sprite, fighter);
        return;
    }

    if (fighter->state == VERSUS_FIGHTER_ATTACK) {
        unsigned_sprite_tick(&fighter->sprite, fighter);
        if (fighter->sprite.state == U_SPRITE_COMPLETED) {
            fighter->state = VERSUS_FIGHTER_IDLE;
            fighter->attack = VERSUS_ATTACK_NONE;
            fighter_play(fighter, VERSUS_ANIM_IDLE, U_SPRITE_PLAY_LOOP);
        }
        return;
    }

    if (fighter->state == VERSUS_FIGHTER_JUMP) {
        fighter->position.x = clamp_s16((s16)(fighter->position.x + fighter->velocity.x), VERSUS_STAGE_LEFT, VERSUS_STAGE_RIGHT);
        fighter->position.y = (s16)(fighter->position.y + fighter->velocity.y);
        fighter->velocity.y = (s16)(fighter->velocity.y + VERSUS_GRAVITY);
        if (fighter->position.y >= VERSUS_GROUND_Y) {
            fighter->position.y = VERSUS_GROUND_Y;
            fighter->velocity = (Vec2){ 0 };
            fighter->state = VERSUS_FIGHTER_IDLE;
            fighter_play(fighter, VERSUS_ANIM_IDLE, U_SPRITE_PLAY_LOOP);
        }
        unsigned_sprite_tick(&fighter->sprite, fighter);
        return;
    }

    if (!controls_enabled) {
        fighter->state = VERSUS_FIGHTER_IDLE;
        fighter_play(fighter, VERSUS_ANIM_IDLE, U_SPRITE_PLAY_LOOP);
        unsigned_sprite_tick(&fighter->sprite, fighter);
        return;
    }

    if (versus_fighter_is_blocking(fighter, controller)) {
        fighter->state = VERSUS_FIGHTER_BLOCK;
        fighter_play(fighter, VERSUS_ANIM_BLOCK, U_SPRITE_PLAY_LOOP);
    } else if ((controller->state.pressed & U_INPUT_BUTTON_A) != 0u && fighter_has_qcf(fighter)) {
        fighter_start_attack(fighter, VERSUS_ATTACK_SPECIAL);
    } else if ((controller->state.pressed & U_INPUT_BUTTON_B) != 0u) {
        fighter_start_attack(fighter, VERSUS_ATTACK_HEAVY);
    } else if ((controller->state.pressed & U_INPUT_BUTTON_A) != 0u) {
        fighter_start_attack(fighter, VERSUS_ATTACK_LIGHT);
    } else if ((controller->state.pressed & U_INPUT_BUTTON_UP) != 0u) {
        fighter->state = VERSUS_FIGHTER_JUMP;
        fighter->velocity.y = VERSUS_JUMP_SPEED;
        fighter->velocity.x = (controller->state.down & U_INPUT_BUTTON_RIGHT) ? 2 : (controller->state.down & U_INPUT_BUTTON_LEFT) ? -2 : 0;
        fighter_play(fighter, VERSUS_ANIM_JUMP, U_SPRITE_PLAY_LOOP);
    } else if ((controller->state.down & U_INPUT_BUTTON_DOWN) != 0u) {
        fighter->state = VERSUS_FIGHTER_CROUCH;
        fighter_play(fighter, VERSUS_ANIM_CROUCH, U_SPRITE_PLAY_LOOP);
    } else {
        s16 dx = 0;
        if ((controller->state.down & U_INPUT_BUTTON_LEFT) != 0u) dx = -VERSUS_WALK_SPEED;
        if ((controller->state.down & U_INPUT_BUTTON_RIGHT) != 0u) dx = VERSUS_WALK_SPEED;
        fighter->position.x = clamp_s16((s16)(fighter->position.x + dx), VERSUS_STAGE_LEFT, VERSUS_STAGE_RIGHT);
        fighter->state = dx == 0 ? VERSUS_FIGHTER_IDLE : VERSUS_FIGHTER_WALK;
        fighter_play(fighter, dx == 0 ? VERSUS_ANIM_IDLE : VERSUS_ANIM_WALK, U_SPRITE_PLAY_LOOP);
    }

    unsigned_sprite_tick(&fighter->sprite, fighter);
}

void versus_fighter_apply_hit(VersusFighter *fighter, s16 damage, u8 hitstun, s16 push_x) {
    if (fighter == NULL || fighter->state == VERSUS_FIGHTER_KO) return;
    fighter->health = fighter->health > damage ? (s16)(fighter->health - damage) : 0;
    fighter->velocity.x = push_x;
    fighter->hitstun_frames = hitstun;
    fighter->state = fighter->health == 0 ? VERSUS_FIGHTER_KO : VERSUS_FIGHTER_HITSTUN;
    fighter_play(fighter, fighter->health == 0 ? VERSUS_ANIM_KO : VERSUS_ANIM_HIT, fighter->health == 0 ? U_SPRITE_PLAY_ONCE : U_SPRITE_PLAY_LOOP);
}

void versus_fighter_apply_block(VersusFighter *fighter, u8 blockstun, s16 push_x) {
    if (fighter == NULL || fighter->state == VERSUS_FIGHTER_KO) return;
    fighter->velocity.x = push_x;
    fighter->hitstun_frames = blockstun;
    fighter->state = VERSUS_FIGHTER_BLOCK;
    fighter_play(fighter, VERSUS_ANIM_BLOCK, U_SPRITE_PLAY_LOOP);
}

static const UCollisionBox *fighter_place_box(VersusFighter *fighter, const UCollisionBox *source, bool hitbox) {
    static UCollisionBox scratch[2];
    UCollisionBox *box = &scratch[hitbox ? 0 : 1];
    if (fighter == NULL || source == NULL) return NULL;
    *box = *source;
    if (!fighter->facing_right) box->offset_x = (s16)(-source->offset_x - source->w);
    box->x = (s16)(fighter->position.x + box->offset_x);
    box->y = (s16)(fighter->position.y + box->offset_y);
    return box;
}

const UCollisionBox *versus_fighter_hitbox(VersusFighter *fighter) {
    const UFrame *frame = fighter == NULL ? NULL : unsigned_sprite_current_frame(&fighter->sprite);
    return frame == NULL ? NULL : fighter_place_box(fighter, frame->hitbox, true);
}

const UCollisionBox *versus_fighter_hurtbox(VersusFighter *fighter) {
    const UFrame *frame = fighter == NULL ? NULL : unsigned_sprite_current_frame(&fighter->sprite);
    return frame == NULL ? NULL : fighter_place_box(fighter, frame->hurtbox, false);
}
