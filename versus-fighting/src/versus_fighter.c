#include "versus_fighter_internal.h"

static s16 clamp_s16(s16 value, s16 minimum, s16 maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static void start_attack(VersusFighter *fighter, VersusAttackKind attack) {
    fighter->attack = attack;
    fighter->attack_connected = false;
    versus_fighter_set_state(fighter, VERSUS_FIGHTER_ATTACK);
}

bool versus_fighter_init(
    VersusFighter *fighter,
    const USpriteDefinition *sprite_definition,
    u16 first_sprite,
    u8 controller_index,
    Vec2 position,
    bool facing_right
) {
    if (fighter == NULL || sprite_definition == NULL) return false;
    *fighter = (VersusFighter){ 0 };

    fighter->player.character = &fighter->character;
    fighter->player.controller_index = controller_index;

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

    if (!versus_fighter_states_init(fighter)) return false;

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
    versus_fighter_input_reset(fighter);

    unsigned_sprite_set_flip_x(&actor->sprite, !facing_right);
    versus_fighter_set_state(fighter, VERSUS_FIGHTER_IDLE);
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

    const VersusFighterState state = versus_fighter_state(fighter);
    if (!state_can_block(state)) return false;

    const UInputMask back = fighter->character.facing_right ? U_INPUT_BUTTON_LEFT : U_INPUT_BUTTON_RIGHT;
    return (controller->state.down & back) != 0u;
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
        versus_fighter_set_state(fighter, VERSUS_FIGHTER_IDLE);
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
        versus_fighter_set_state(fighter, VERSUS_FIGHTER_IDLE);
    }
}

static void update_ground_controls(VersusFighter *fighter, const UInputController *controller) {
    if (versus_fighter_is_blocking(fighter, controller)) {
        versus_fighter_set_state(fighter, VERSUS_FIGHTER_BLOCK);
        return;
    }

    if ((controller->state.pressed & U_INPUT_BUTTON_A) != 0u && versus_fighter_input_has_qcf(fighter)) {
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
        versus_fighter_set_state(fighter, VERSUS_FIGHTER_JUMP);
        return;
    }
    if ((controller->state.down & U_INPUT_BUTTON_DOWN) != 0u) {
        versus_fighter_set_state(fighter, VERSUS_FIGHTER_CROUCH);
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
    versus_fighter_set_state(fighter, dx == 0 ? VERSUS_FIGHTER_IDLE : VERSUS_FIGHTER_WALK);
}

void versus_fighter_update(VersusFighter *fighter, const UInputController *controller, bool controls_enabled) {
    if (fighter == NULL || controller == NULL) return;

    versus_fighter_input_push(fighter, controller);

    const VersusFighterState state = versus_fighter_state(fighter);
    if (state == VERSUS_FIGHTER_KO) return;

    if (fighter->stun_frames > 0u) {
        update_stun(fighter);
        return;
    }

    if (state == VERSUS_FIGHTER_ATTACK) {
        if (fighter->character.actor.sprite.state == U_SPRITE_COMPLETED) {
            fighter->attack = VERSUS_ATTACK_NONE;
            versus_fighter_set_state(fighter, VERSUS_FIGHTER_IDLE);
        }
        return;
    }

    if (state == VERSUS_FIGHTER_JUMP) {
        update_jump(fighter);
        return;
    }

    if (!controls_enabled) {
        versus_fighter_set_state(fighter, VERSUS_FIGHTER_IDLE);
        return;
    }

    update_ground_controls(fighter, controller);
}

void versus_fighter_apply_hit(VersusFighter *fighter, const VersusAttackDefinition *attack, s16 direction) {
    if (fighter == NULL || attack == NULL || versus_fighter_state(fighter) == VERSUS_FIGHTER_KO) return;

    UGameplayAttribute *health = &fighter->attributes[VERSUS_ATTRIBUTE_HEALTH];
    health->current_value = health->current_value > attack->damage
        ? (s16)(health->current_value - attack->damage)
        : 0;

    fighter->velocity.x = (s16)(direction * attack->pushback);
    fighter->stun_frames = attack->hitstun_frames;
    versus_fighter_set_state(
        fighter,
        health->current_value == 0 ? VERSUS_FIGHTER_KO : VERSUS_FIGHTER_HITSTUN
    );
}

void versus_fighter_apply_block(VersusFighter *fighter, const VersusAttackDefinition *attack, s16 direction) {
    if (fighter == NULL || attack == NULL || versus_fighter_state(fighter) == VERSUS_FIGHTER_KO) return;

    fighter->velocity.x = (s16)(direction * attack->pushback);
    fighter->stun_frames = attack->blockstun_frames;
    versus_fighter_set_state(fighter, VERSUS_FIGHTER_BLOCK);
}

s16 versus_fighter_health(const VersusFighter *fighter) {
    return fighter != NULL ? fighter->attributes[VERSUS_ATTRIBUTE_HEALTH].current_value : 0;
}
