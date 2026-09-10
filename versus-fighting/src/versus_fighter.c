#include "versus_fighter.h"

#include "actor/collision_state.h"

/* Numeric keypad notation used by traditional fighting-game command parsers. */
enum {
    DIR_DOWN = 2,
    DIR_DOWN_FORWARD = 3,
    DIR_FORWARD = 6,
};

static s16 clamp_s16(s16 value, s16 minimum, s16 maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

/* ------------------------------------------------------------------------- */
/* State and animation                                                       */
/* ------------------------------------------------------------------------- */

VersusFighterState versus_fighter_state(const VersusFighter *fighter) {
    return fighter != NULL
        ? (VersusFighterState)versus_state_machine_current_index(&fighter->states)
        : VERSUS_FIGHTER_IDLE;
}

static void play_animation(VersusFighter *fighter, VersusAnimation animation, UAnimationPlayback playback) {
    USprite *sprite = &fighter->character.actor.sprite;
    if (sprite->animation_index != (u8)animation || sprite->state == U_SPRITE_COMPLETED) {
        (void)unsigned_sprite_play(sprite, (u8)animation, playback);
    }
}

static void play_state_animation(VersusFighter *fighter, VersusFighterState state) {
    switch (state) {
        case VERSUS_FIGHTER_IDLE:
            play_animation(fighter, VERSUS_ANIM_IDLE, U_SPRITE_PLAY_LOOP);
            break;
        case VERSUS_FIGHTER_WALK:
            play_animation(fighter, VERSUS_ANIM_WALK, U_SPRITE_PLAY_LOOP);
            break;
        case VERSUS_FIGHTER_CROUCH:
            play_animation(fighter, VERSUS_ANIM_CROUCH, U_SPRITE_PLAY_LOOP);
            break;
        case VERSUS_FIGHTER_JUMP:
            play_animation(fighter, VERSUS_ANIM_JUMP, U_SPRITE_PLAY_LOOP);
            break;
        case VERSUS_FIGHTER_BLOCK:
            play_animation(fighter, VERSUS_ANIM_BLOCK, U_SPRITE_PLAY_LOOP);
            break;
        case VERSUS_FIGHTER_HITSTUN:
            play_animation(fighter, VERSUS_ANIM_HIT, U_SPRITE_PLAY_LOOP);
            break;
        case VERSUS_FIGHTER_KO:
            play_animation(fighter, VERSUS_ANIM_KO, U_SPRITE_PLAY_ONCE);
            break;
        case VERSUS_FIGHTER_ATTACK: {
            const VersusAttackDefinition *attack = versus_content_attack(fighter->attack);
            if (attack != NULL) {
                play_animation(fighter, attack->animation, U_SPRITE_PLAY_ONCE);
            }
            break;
        }
        default:
            break;
    }
}

static void state_enter(UStateGraph *graph, void *context) {
    VersusFighter *fighter = context;
    (void)graph;
    if (fighter != NULL) {
        play_state_animation(fighter, versus_fighter_state(fighter));
    }
}

static bool states_init(VersusFighter *fighter) {
    return versus_state_machine_init(
        &fighter->states,
        VERSUS_FIGHTER_STATE_COUNT,
        VERSUS_FIGHTER_IDLE,
        state_enter,
        fighter
    );
}

static void set_state(VersusFighter *fighter, VersusFighterState state) {
    if (fighter != NULL && state < VERSUS_FIGHTER_STATE_COUNT) {
        versus_state_machine_set(&fighter->states, (u8)state);
    }
}

/* ------------------------------------------------------------------------- */
/* Command input                                                             */
/* ------------------------------------------------------------------------- */

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

static u8 history_index(const VersusInputBuffer *buffer, u8 age) {
    return (u8)((u8)(buffer->head - age) & VERSUS_INPUT_BUFFER_MASK);
}

static void input_reset(VersusFighter *fighter) {
    fighter->input_buffer = (VersusInputBuffer){ 0 };
}

static void input_push(VersusFighter *fighter, const UInputController *controller) {
    fighter->input_buffer.head = (u8)((fighter->input_buffer.head + 1u) & VERSUS_INPUT_BUFFER_MASK);
    fighter->input_buffer.samples[fighter->input_buffer.head].direction = relative_direction(fighter, controller);
}

static bool input_has_qcf(const VersusFighter *fighter) {
    bool saw_forward = false;
    bool saw_down_forward = false;

    for (u8 age = 0u; age < VERSUS_QCF_WINDOW; ++age) {
        const u8 direction = fighter->input_buffer.samples[history_index(&fighter->input_buffer, age)].direction;

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

/* ------------------------------------------------------------------------- */
/* Fighter lifecycle                                                         */
/* ------------------------------------------------------------------------- */

bool versus_fighter_init(
    VersusFighter *fighter,
    const USpriteDefinition *sprite_definition,
    u16 first_sprite,
    Vec2 position,
    bool facing_right
) {
    if (fighter == NULL || sprite_definition == NULL) return false;
    *fighter = (VersusFighter){ 0 };

    fighter->player.character = &fighter->character;
    fighter->character.attributes = (UGameplayAttributeContainer){
        .count = VERSUS_ATTRIBUTE_COUNT,
        .capacity = VERSUS_ATTRIBUTE_COUNT,
        .instances = fighter->attributes,
    };
    fighter->attributes[VERSUS_ATTRIBUTE_HEALTH] = (UGameplayAttribute){
        .base_value = VERSUS_MAX_HEALTH,
        .current_value = VERSUS_MAX_HEALTH,
    };

    if (!unsigned_sprite_init(&fighter->character.actor.sprite, sprite_definition, first_sprite)) return false;
    fighter->character.actor.sprite.offset = (Vec2){
        .x = VERSUS_SPRITE_ANCHOR_X,
        .y = VERSUS_SPRITE_ANCHOR_Y,
    };

    if (!states_init(fighter)) return false;

    versus_fighter_reset(fighter, position, facing_right);
    return true;
}

void versus_fighter_reset(VersusFighter *fighter, Vec2 position, bool facing_right) {
    if (fighter == NULL) return;

    UActor *actor = &fighter->character.actor;
    actor->position = position;
    actor->active = true;
    unsigned_actor_collision_state_reset(&actor->collision);

    fighter->character.facing_right = facing_right ? 1u : 0u;
    fighter->velocity = (Vec2){ 0 };
    fighter->attack = VERSUS_ATTACK_NONE;
    fighter->stun_frames = 0u;
    fighter->attack_connected = false;
    fighter->attributes[VERSUS_ATTRIBUTE_HEALTH].current_value = VERSUS_MAX_HEALTH;
    input_reset(fighter);

    unsigned_sprite_set_flip_x(&actor->sprite, !facing_right);
    set_state(fighter, VERSUS_FIGHTER_IDLE);
    (void)unsigned_sprite_play(&actor->sprite, VERSUS_ANIM_IDLE, U_SPRITE_PLAY_LOOP);
}

void versus_fighter_face_opponent(VersusFighter *fighter, const VersusFighter *opponent) {
    if (fighter == NULL || opponent == NULL || versus_fighter_state(fighter) == VERSUS_FIGHTER_KO) return;

    fighter->character.facing_right =
        fighter->character.actor.position.x <= opponent->character.actor.position.x ? 1u : 0u;
    unsigned_sprite_set_flip_x(
        &fighter->character.actor.sprite,
        fighter->character.facing_right == 0u
    );
}

static bool state_can_block(VersusFighterState state) {
    return state == VERSUS_FIGHTER_IDLE ||
           state == VERSUS_FIGHTER_WALK ||
           state == VERSUS_FIGHTER_CROUCH ||
           state == VERSUS_FIGHTER_BLOCK;
}

bool versus_fighter_is_blocking(const VersusFighter *fighter, const UInputController *controller) {
    if (fighter == NULL || controller == NULL) return false;

    if (!state_can_block(versus_fighter_state(fighter))) return false;

    const UInputMask back = fighter->character.facing_right ? U_INPUT_BUTTON_LEFT : U_INPUT_BUTTON_RIGHT;
    return (controller->state.down & back) != 0u;
}

/* ------------------------------------------------------------------------- */
/* Per-frame behavior                                                        */
/* ------------------------------------------------------------------------- */

static void start_attack(VersusFighter *fighter, VersusAttackKind attack) {
    fighter->attack = attack;
    fighter->attack_connected = false;

    /* Consume a directional command once accepted so stale history cannot
       trigger another special on a later A press. */
    if (attack == VERSUS_ATTACK_SPECIAL) {
        input_reset(fighter);
    }

    set_state(fighter, VERSUS_FIGHTER_ATTACK);
}

static void update_stun(VersusFighter *fighter) {
    UActor *actor = &fighter->character.actor;

    --fighter->stun_frames;
    actor->position.x = clamp_s16(
        (s16)(actor->position.x + fighter->velocity.x),
        VERSUS_STAGE_LEFT,
        VERSUS_STAGE_RIGHT
    );
    fighter->velocity.x = 0;

    if (fighter->stun_frames == 0u) {
        set_state(fighter, VERSUS_FIGHTER_IDLE);
    }
}

static void update_jump(VersusFighter *fighter) {
    UActor *actor = &fighter->character.actor;

    actor->position.x = clamp_s16(
        (s16)(actor->position.x + fighter->velocity.x),
        VERSUS_STAGE_LEFT,
        VERSUS_STAGE_RIGHT
    );
    actor->position.y = (s16)(actor->position.y + fighter->velocity.y);
    fighter->velocity.y = (s16)(fighter->velocity.y + VERSUS_GRAVITY);

    if (actor->position.y >= VERSUS_GROUND_Y) {
        actor->position.y = VERSUS_GROUND_Y;
        fighter->velocity = (Vec2){ 0 };
        set_state(fighter, VERSUS_FIGHTER_IDLE);
    }
}

static void update_ground_controls(VersusFighter *fighter, const UInputController *controller) {
    if (versus_fighter_is_blocking(fighter, controller)) {
        set_state(fighter, VERSUS_FIGHTER_BLOCK);
        return;
    }

    if ((controller->state.pressed & U_INPUT_BUTTON_A) != 0u && input_has_qcf(fighter)) {
        start_attack(fighter, VERSUS_ATTACK_SPECIAL);
        return;
    }
    if ((controller->state.pressed & U_INPUT_BUTTON_B) != 0u) {
        start_attack(fighter, VERSUS_ATTACK_HEAVY);
        return;
    }
    if ((controller->state.pressed & U_INPUT_BUTTON_A) != 0u) {
        start_attack(fighter, VERSUS_ATTACK_LIGHT);
        return;
    }
    if ((controller->state.pressed & U_INPUT_BUTTON_UP) != 0u) {
        fighter->velocity.y = VERSUS_JUMP_SPEED;
        fighter->velocity.x = (controller->state.down & U_INPUT_BUTTON_RIGHT) ? VERSUS_JUMP_HORIZONTAL_SPEED :
                              (controller->state.down & U_INPUT_BUTTON_LEFT) ? -VERSUS_JUMP_HORIZONTAL_SPEED : 0;
        set_state(fighter, VERSUS_FIGHTER_JUMP);
        return;
    }
    if ((controller->state.down & U_INPUT_BUTTON_DOWN) != 0u) {
        set_state(fighter, VERSUS_FIGHTER_CROUCH);
        return;
    }

    s16 dx = 0;
    if ((controller->state.down & U_INPUT_BUTTON_LEFT) != 0u) dx = -VERSUS_WALK_SPEED;
    if ((controller->state.down & U_INPUT_BUTTON_RIGHT) != 0u) dx = VERSUS_WALK_SPEED;

    fighter->character.actor.position.x = clamp_s16(
        (s16)(fighter->character.actor.position.x + dx),
        VERSUS_STAGE_LEFT,
        VERSUS_STAGE_RIGHT
    );
    set_state(fighter, dx == 0 ? VERSUS_FIGHTER_IDLE : VERSUS_FIGHTER_WALK);
}

void versus_fighter_update(VersusFighter *fighter, const UInputController *controller, bool controls_enabled) {
    if (fighter == NULL || controller == NULL) return;

    input_push(fighter, controller);

    const VersusFighterState state = versus_fighter_state(fighter);
    if (state == VERSUS_FIGHTER_KO) return;

    if (fighter->stun_frames > 0u) {
        update_stun(fighter);
        return;
    }

    if (state == VERSUS_FIGHTER_ATTACK) {
        if (fighter->character.actor.sprite.state == U_SPRITE_COMPLETED) {
            fighter->attack = VERSUS_ATTACK_NONE;
            set_state(fighter, VERSUS_FIGHTER_IDLE);
        }
        return;
    }

    if (state == VERSUS_FIGHTER_JUMP) {
        update_jump(fighter);
        return;
    }

    if (!controls_enabled) {
        set_state(fighter, VERSUS_FIGHTER_IDLE);
        return;
    }

    update_ground_controls(fighter, controller);
}

/* ------------------------------------------------------------------------- */
/* Combat reactions                                                          */
/* ------------------------------------------------------------------------- */

void versus_fighter_apply_hit(VersusFighter *fighter, const VersusAttackDefinition *attack, s16 direction) {
    if (fighter == NULL || attack == NULL || versus_fighter_state(fighter) == VERSUS_FIGHTER_KO) return;

    UGameplayAttribute *health = &fighter->attributes[VERSUS_ATTRIBUTE_HEALTH];
    health->current_value = health->current_value > attack->damage
        ? (s16)(health->current_value - attack->damage)
        : 0;

    fighter->velocity.x = (s16)(direction * attack->pushback);
    fighter->stun_frames = attack->hitstun_frames;
    set_state(
        fighter,
        health->current_value == 0 ? VERSUS_FIGHTER_KO : VERSUS_FIGHTER_HITSTUN
    );
}

void versus_fighter_apply_block(VersusFighter *fighter, const VersusAttackDefinition *attack, s16 direction) {
    if (fighter == NULL || attack == NULL || versus_fighter_state(fighter) == VERSUS_FIGHTER_KO) return;

    fighter->velocity.x = (s16)(direction * attack->pushback);
    fighter->stun_frames = attack->blockstun_frames;
    set_state(fighter, VERSUS_FIGHTER_BLOCK);
}

s16 versus_fighter_health(const VersusFighter *fighter) {
    return fighter != NULL ? fighter->attributes[VERSUS_ATTRIBUTE_HEALTH].current_value : 0;
}
