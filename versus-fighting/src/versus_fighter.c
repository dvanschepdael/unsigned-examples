#include "versus_fighter.h"

#define VERSUS_GROUND_Y 184
#define VERSUS_STAGE_LEFT 24
#define VERSUS_STAGE_RIGHT 296
#define VERSUS_WALK_SPEED 2
#define VERSUS_JUMP_SPEED (-7)
#define VERSUS_GRAVITY 1
#define VERSUS_SPRITE_ANCHOR_X (-32)
#define VERSUS_SPRITE_ANCHOR_Y (-64)

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

static USprite *fighter_sprite(VersusFighter *fighter) {
    return &fighter->character.actor.sprite;
}

static const USprite *fighter_sprite_const(const VersusFighter *fighter) {
    return &fighter->character.actor.sprite;
}

static Vec2 *fighter_position(VersusFighter *fighter) {
    return &fighter->character.actor.position;
}

static const Vec2 *fighter_position_const(const VersusFighter *fighter) {
    return &fighter->character.actor.position;
}

static void fighter_play(VersusFighter *fighter, VersusAnimation animation, UAnimationPlayback playback) {
    USprite *sprite = fighter_sprite(fighter);
    if (sprite->animation_index != animation || sprite->state == U_SPRITE_COMPLETED) {
        (void)unsigned_sprite_play(sprite, (u8)animation, playback);
    }
}

static VersusFighterState fighter_graph_state(const VersusFighter *fighter, const UStateGraphNode *node) {
    if (node == NULL) return VERSUS_FIGHTER_IDLE;
    for (u8 i = 0u; i < VERSUS_FIGHTER_STATE_COUNT; ++i) {
        if (node == &fighter->state_nodes[i]) return (VersusFighterState)i;
    }
    return VERSUS_FIGHTER_IDLE;
}

static void fighter_state_enter(UStateGraph *graph, void *context) {
    VersusFighter *fighter = context;
    VersusFighterState state;
    if (fighter == NULL || graph == NULL) return;

    state = fighter_graph_state(fighter, graph->current);
    fighter->state = state;

    switch (state) {
        case VERSUS_FIGHTER_IDLE: fighter_play(fighter, VERSUS_ANIM_IDLE, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_WALK: fighter_play(fighter, VERSUS_ANIM_WALK, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_CROUCH: fighter_play(fighter, VERSUS_ANIM_CROUCH, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_JUMP: fighter_play(fighter, VERSUS_ANIM_JUMP, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_BLOCK: fighter_play(fighter, VERSUS_ANIM_BLOCK, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_HITSTUN: fighter_play(fighter, VERSUS_ANIM_HIT, U_SPRITE_PLAY_LOOP); break;
        case VERSUS_FIGHTER_KO: fighter_play(fighter, VERSUS_ANIM_KO, U_SPRITE_PLAY_ONCE); break;
        case VERSUS_FIGHTER_ATTACK:
            if (fighter->attack == VERSUS_ATTACK_LIGHT) fighter_play(fighter, VERSUS_ANIM_LIGHT, U_SPRITE_PLAY_ONCE);
            else if (fighter->attack == VERSUS_ATTACK_HEAVY) fighter_play(fighter, VERSUS_ANIM_HEAVY, U_SPRITE_PLAY_ONCE);
            else if (fighter->attack == VERSUS_ATTACK_SPECIAL) fighter_play(fighter, VERSUS_ANIM_SPECIAL, U_SPRITE_PLAY_ONCE);
            break;
        default: break;
    }
}

static bool fighter_state_graph_init(VersusFighter *fighter) {
    fighter->state_root = (UStateGraphNode){ 0 };
    fighter->state_children = (UStateGraphNodeContainer){
        .count = VERSUS_FIGHTER_STATE_COUNT,
        .capacity = VERSUS_FIGHTER_STATE_COUNT,
        .instances = fighter->state_nodes,
    };
    fighter->state_event_container = (UStateGraphTransitionContainer){
        .count = VERSUS_FIGHTER_STATE_COUNT,
        .capacity = VERSUS_FIGHTER_STATE_COUNT,
        .instances = fighter->state_events,
    };
    fighter->state_root.edges = &fighter->state_children;
    fighter->state_root.transitions[U_TRANSITION_ON_EVENT] = &fighter->state_event_container;

    for (u8 i = 0u; i < VERSUS_FIGHTER_STATE_COUNT; ++i) {
        fighter->state_nodes[i] = (UStateGraphNode){
            .parent = &fighter->state_root,
            .enter = fighter_state_enter,
        };
        fighter->state_events[i] = (UStateGraphTransition){
            .target = &fighter->state_nodes[i],
        };
    }

    return unsigned_state_graph_init(&fighter->state_graph, &fighter->state_root, &fighter->state_nodes[VERSUS_FIGHTER_IDLE], fighter);
}

static void fighter_set_state(VersusFighter *fighter, VersusFighterState state) {
    if (fighter == NULL || state >= VERSUS_FIGHTER_STATE_COUNT || fighter->state == state) return;
    unsigned_state_graph_send_event(&fighter->state_graph, (UEvent)state);
}

static u8 fighter_relative_direction(const VersusFighter *fighter, const UInputController *controller) {
    bool up = (controller->state.down & U_INPUT_BUTTON_UP) != 0u;
    bool down = (controller->state.down & U_INPUT_BUTTON_DOWN) != 0u;
    bool left = (controller->state.down & U_INPUT_BUTTON_LEFT) != 0u;
    bool right = (controller->state.down & U_INPUT_BUTTON_RIGHT) != 0u;
    bool forward = fighter->character.facing_right ? right : left;
    bool back = fighter->character.facing_right ? left : right;

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
        if (!saw_forward && direction == DIR_FORWARD) saw_forward = true;
        else if (saw_forward && !saw_down_forward && direction == DIR_DOWN_FORWARD) saw_down_forward = true;
        else if (saw_down_forward && direction == DIR_DOWN) return true;
    }
    return false;
}

static void fighter_start_attack(VersusFighter *fighter, VersusAttackKind attack) {
    fighter->attack = attack;
    fighter->attack_connected = 0u;
    fighter_set_state(fighter, VERSUS_FIGHTER_ATTACK);
}

bool versus_fighter_init(VersusFighter *fighter, const USpriteDefinition *sprite_definition, u16 first_sprite, u8 controller_index, s16 x, s16 y, bool facing_right) {
    if (fighter == NULL || sprite_definition == NULL) return false;
    *fighter = (VersusFighter){ 0 };

    fighter->player.character = &fighter->character;
    fighter->player.controller_index = controller_index;
    fighter->character.actor.position = (Vec2){ .x = x, .y = y };
    fighter->character.facing_right = facing_right ? 1u : 0u;

    if (!unsigned_sprite_init(&fighter->character.actor.sprite, sprite_definition, first_sprite)) return false;
    fighter->character.actor.sprite.offset = (Vec2){ .x = VERSUS_SPRITE_ANCHOR_X, .y = VERSUS_SPRITE_ANCHOR_Y };
    if (!fighter_state_graph_init(fighter)) return false;

    versus_fighter_reset(fighter, x, y, facing_right);
    return true;
}

void versus_fighter_reset(VersusFighter *fighter, s16 x, s16 y, bool facing_right) {
    if (fighter == NULL) return;
    fighter->character.actor.position = (Vec2){ .x = x, .y = y };
    fighter->character.actor.active = true;
    fighter->character.facing_right = facing_right ? 1u : 0u;
    fighter->velocity = (Vec2){ 0 };
    fighter->input_buffer = (VersusInputBuffer){ 0 };
    fighter->attack = VERSUS_ATTACK_NONE;
    fighter->health = 100;
    fighter->hitstun_frames = 0u;
    fighter->attack_connected = 0u;
    unsigned_sprite_set_flip_x(fighter_sprite(fighter), !facing_right);
    if (fighter->state_graph.current != &fighter->state_nodes[VERSUS_FIGHTER_IDLE]) {
        unsigned_state_graph_send_event(&fighter->state_graph, VERSUS_FIGHTER_IDLE);
    } else {
        fighter->state = VERSUS_FIGHTER_IDLE;
        fighter_play(fighter, VERSUS_ANIM_IDLE, U_SPRITE_PLAY_LOOP);
    }
}

void versus_fighter_face_opponent(VersusFighter *fighter, const VersusFighter *opponent) {
    const Vec2 *self_pos;
    const Vec2 *other_pos;
    if (fighter == NULL || opponent == NULL || fighter->state == VERSUS_FIGHTER_KO) return;
    self_pos = fighter_position_const(fighter);
    other_pos = fighter_position_const(opponent);
    fighter->character.facing_right = self_pos->x <= other_pos->x ? 1u : 0u;
    unsigned_sprite_set_flip_x(fighter_sprite(fighter), fighter->character.facing_right == 0u);
}

bool versus_fighter_is_blocking(const VersusFighter *fighter, const UInputController *controller) {
    UInputMask back;
    if (fighter == NULL || controller == NULL || fighter->state == VERSUS_FIGHTER_KO || fighter->state == VERSUS_FIGHTER_HITSTUN) return false;
    back = fighter->character.facing_right ? U_INPUT_BUTTON_LEFT : U_INPUT_BUTTON_RIGHT;
    return (controller->state.down & back) != 0u;
}

void versus_fighter_update(VersusFighter *fighter, const UInputController *controller, bool controls_enabled) {
    Vec2 *position;
    if (fighter == NULL || controller == NULL) return;
    position = fighter_position(fighter);
    fighter_buffer_push(fighter, controller);

    if (fighter->state == VERSUS_FIGHTER_KO) return;

    if (fighter->hitstun_frames > 0u) {
        --fighter->hitstun_frames;
        position->x = clamp_s16((s16)(position->x + fighter->velocity.x), VERSUS_STAGE_LEFT, VERSUS_STAGE_RIGHT);
        fighter->velocity.x = 0;
        if (fighter->hitstun_frames == 0u) fighter_set_state(fighter, VERSUS_FIGHTER_IDLE);
        return;
    }

    if (fighter->state == VERSUS_FIGHTER_ATTACK) {
        if (fighter_sprite_const(fighter)->state == U_SPRITE_COMPLETED) {
            fighter->attack = VERSUS_ATTACK_NONE;
            fighter_set_state(fighter, VERSUS_FIGHTER_IDLE);
        }
        return;
    }

    if (fighter->state == VERSUS_FIGHTER_JUMP) {
        position->x = clamp_s16((s16)(position->x + fighter->velocity.x), VERSUS_STAGE_LEFT, VERSUS_STAGE_RIGHT);
        position->y = (s16)(position->y + fighter->velocity.y);
        fighter->velocity.y = (s16)(fighter->velocity.y + VERSUS_GRAVITY);
        if (position->y >= VERSUS_GROUND_Y) {
            position->y = VERSUS_GROUND_Y;
            fighter->velocity = (Vec2){ 0 };
            fighter_set_state(fighter, VERSUS_FIGHTER_IDLE);
        }
        return;
    }

    if (!controls_enabled) {
        fighter_set_state(fighter, VERSUS_FIGHTER_IDLE);
        return;
    }

    if (versus_fighter_is_blocking(fighter, controller)) {
        fighter_set_state(fighter, VERSUS_FIGHTER_BLOCK);
    } else if ((controller->state.pressed & U_INPUT_BUTTON_A) != 0u && fighter_has_qcf(fighter)) {
        fighter_start_attack(fighter, VERSUS_ATTACK_SPECIAL);
    } else if ((controller->state.pressed & U_INPUT_BUTTON_B) != 0u) {
        fighter_start_attack(fighter, VERSUS_ATTACK_HEAVY);
    } else if ((controller->state.pressed & U_INPUT_BUTTON_A) != 0u) {
        fighter_start_attack(fighter, VERSUS_ATTACK_LIGHT);
    } else if ((controller->state.pressed & U_INPUT_BUTTON_UP) != 0u) {
        fighter->velocity.y = VERSUS_JUMP_SPEED;
        fighter->velocity.x = (controller->state.down & U_INPUT_BUTTON_RIGHT) ? 2 : (controller->state.down & U_INPUT_BUTTON_LEFT) ? -2 : 0;
        fighter_set_state(fighter, VERSUS_FIGHTER_JUMP);
    } else if ((controller->state.down & U_INPUT_BUTTON_DOWN) != 0u) {
        fighter_set_state(fighter, VERSUS_FIGHTER_CROUCH);
    } else {
        s16 dx = 0;
        if ((controller->state.down & U_INPUT_BUTTON_LEFT) != 0u) dx = -VERSUS_WALK_SPEED;
        if ((controller->state.down & U_INPUT_BUTTON_RIGHT) != 0u) dx = VERSUS_WALK_SPEED;
        position->x = clamp_s16((s16)(position->x + dx), VERSUS_STAGE_LEFT, VERSUS_STAGE_RIGHT);
        fighter_set_state(fighter, dx == 0 ? VERSUS_FIGHTER_IDLE : VERSUS_FIGHTER_WALK);
    }

    unsigned_state_graph_tick(&fighter->state_graph);
}

void versus_fighter_apply_hit(VersusFighter *fighter, s16 damage, u8 hitstun, s16 push_x) {
    if (fighter == NULL || fighter->state == VERSUS_FIGHTER_KO) return;
    fighter->health = fighter->health > damage ? (s16)(fighter->health - damage) : 0;
    fighter->velocity.x = push_x;
    fighter->hitstun_frames = hitstun;
    fighter_set_state(fighter, fighter->health == 0 ? VERSUS_FIGHTER_KO : VERSUS_FIGHTER_HITSTUN);
}

void versus_fighter_apply_block(VersusFighter *fighter, u8 blockstun, s16 push_x) {
    if (fighter == NULL || fighter->state == VERSUS_FIGHTER_KO) return;
    fighter->velocity.x = push_x;
    fighter->hitstun_frames = blockstun;
    fighter_set_state(fighter, VERSUS_FIGHTER_BLOCK);
}

static const UCollisionBox *fighter_place_box(VersusFighter *fighter, const UCollisionBox *source, bool hitbox) {
    static UCollisionBox scratch[2];
    UCollisionBox *box = &scratch[hitbox ? 0 : 1];
    const Vec2 *position;
    if (fighter == NULL || source == NULL) return NULL;
    position = fighter_position_const(fighter);
    *box = *source;
    if (!fighter->character.facing_right) box->offset_x = (s16)(-source->offset_x - source->w);
    box->x = (s16)(position->x + box->offset_x);
    box->y = (s16)(position->y + box->offset_y);
    return box;
}

const UCollisionBox *versus_fighter_hitbox(VersusFighter *fighter) {
    const UFrame *frame = fighter == NULL ? NULL : unsigned_sprite_current_frame(fighter_sprite(fighter));
    return frame == NULL ? NULL : fighter_place_box(fighter, frame->hitbox, true);
}

const UCollisionBox *versus_fighter_hurtbox(VersusFighter *fighter) {
    const UFrame *frame = fighter == NULL ? NULL : unsigned_sprite_current_frame(fighter_sprite(fighter));
    return frame == NULL ? NULL : fighter_place_box(fighter, frame->hurtbox, false);
}
