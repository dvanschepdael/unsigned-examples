#include "versus_match.h"

#include "physics/collision_box.h"
#include "system/renderer/sprite_renderer.h"

#define ROUND_SECONDS 60u
#define INTRO_SECONDS 2u
#define OUTRO_SECONDS 2u
#define ROUNDS_TO_WIN 2u
#define MIN_SEPARATION 24

static void match_begin_round(VersusMatch *match) {
    versus_fighter_reset(&match->fighters[0], 92, 184, true);
    versus_fighter_reset(&match->fighters[1], 228, 184, false);
    match->phase = VERSUS_ROUND_INTRO;
    match->phase_frames = (u16)(match->refresh_rate * INTRO_SECONDS);
    match->round_frames_remaining = (u16)(match->refresh_rate * ROUND_SECONDS);
    match->hitstop_frames = 0u;
    ++match->round_number;
}

bool versus_match_init(VersusMatch *match, u16 refresh_rate) {
    if (match == NULL || refresh_rate == 0u) return false;
    *match = (VersusMatch){ .refresh_rate = refresh_rate };
    if (!versus_fighter_init(&match->fighters[0], &VERSUS_FIGHTER_P1_SPRITE, 1, 0, 92, 184, true)) return false;
    if (!versus_fighter_init(&match->fighters[1], &VERSUS_FIGHTER_P2_SPRITE, 5, 1, 228, 184, false)) return false;
    return true;
}

void versus_match_start(VersusMatch *match) {
    if (match == NULL) return;
    match->rounds_won[0] = 0u;
    match->rounds_won[1] = 0u;
    match->round_number = 0u;
    match->winner = 0xffu;
    match_begin_round(match);
}

static s16 attack_damage(VersusAttackKind attack) {
    switch (attack) {
        case VERSUS_ATTACK_LIGHT: return 8;
        case VERSUS_ATTACK_HEAVY: return 14;
        case VERSUS_ATTACK_SPECIAL: return 20;
        default: return 0;
    }
}

static u8 attack_hitstun(VersusAttackKind attack) {
    switch (attack) {
        case VERSUS_ATTACK_LIGHT: return 10;
        case VERSUS_ATTACK_HEAVY: return 16;
        case VERSUS_ATTACK_SPECIAL: return 22;
        default: return 0;
    }
}

static void match_resolve_attack(VersusMatch *match, u8 attacker_index, const UInputManager *input) {
    u8 defender_index = (u8)(attacker_index ^ 1u);
    VersusFighter *attacker = &match->fighters[attacker_index];
    VersusFighter *defender = &match->fighters[defender_index];
    const UCollisionBox *hitbox;
    const UCollisionBox *hurtbox;
    s16 direction;

    if (attacker->attack == VERSUS_ATTACK_NONE || attacker->attack_connected) return;
    hitbox = versus_fighter_hitbox(attacker);
    hurtbox = versus_fighter_hurtbox(defender);
    if (hitbox == NULL || hurtbox == NULL || !unsigned_physics_collision_box_intersects_fast(hitbox, hurtbox)) return;

    attacker->attack_connected = 1u;
    direction = attacker->facing_right ? 2 : -2;
    if (versus_fighter_is_blocking(defender, &input->players[defender->controller_index])) {
        versus_fighter_apply_block(defender, 7u, direction);
        match->hitstop_frames = 4u;
    } else {
        versus_fighter_apply_hit(defender, attack_damage(attacker->attack), attack_hitstun(attacker->attack), direction);
        match->hitstop_frames = 6u;
    }
}

static void match_resolve_pushboxes(VersusMatch *match) {
    VersusFighter *left = &match->fighters[0];
    VersusFighter *right = &match->fighters[1];
    s16 distance;

    if (left->position.x > right->position.x) {
        left = &match->fighters[1];
        right = &match->fighters[0];
    }
    distance = (s16)(right->position.x - left->position.x);
    if (distance < MIN_SEPARATION) {
        s16 correction = (s16)((MIN_SEPARATION - distance + 1) / 2);
        left->position.x = (s16)(left->position.x - correction);
        right->position.x = (s16)(right->position.x + correction);
    }
}

static void match_finish_round(VersusMatch *match) {
    s16 p1 = match->fighters[0].health;
    s16 p2 = match->fighters[1].health;
    match->winner = p1 == p2 ? 0xffu : (p1 > p2 ? 0u : 1u);
    if (match->winner < 2u) ++match->rounds_won[match->winner];
    match->phase = VERSUS_ROUND_OUTRO;
    match->phase_frames = (u16)(match->refresh_rate * OUTRO_SECONDS);
}

void versus_match_tick(VersusMatch *match, const UInputManager *input) {
    bool controls_enabled;
    if (match == NULL || input == NULL) return;

    if (match->hitstop_frames > 0u) {
        --match->hitstop_frames;
        return;
    }

    if (match->phase == VERSUS_ROUND_INTRO) {
        versus_fighter_tick(&match->fighters[0], &input->players[0], false);
        versus_fighter_tick(&match->fighters[1], &input->players[1], false);
        if (match->phase_frames > 0u) --match->phase_frames;
        if (match->phase_frames == 0u) match->phase = VERSUS_ROUND_FIGHT;
        return;
    }

    if (match->phase == VERSUS_ROUND_OUTRO) {
        versus_fighter_tick(&match->fighters[0], &input->players[0], false);
        versus_fighter_tick(&match->fighters[1], &input->players[1], false);
        if (match->phase_frames > 0u) --match->phase_frames;
        if (match->phase_frames == 0u) {
            if (match->rounds_won[0] >= ROUNDS_TO_WIN || match->rounds_won[1] >= ROUNDS_TO_WIN) match->phase = VERSUS_MATCH_OVER;
            else match_begin_round(match);
        }
        return;
    }

    if (match->phase == VERSUS_MATCH_OVER) {
        versus_fighter_tick(&match->fighters[0], &input->players[0], false);
        versus_fighter_tick(&match->fighters[1], &input->players[1], false);
        return;
    }

    controls_enabled = match->phase == VERSUS_ROUND_FIGHT;
    versus_fighter_face_opponent(&match->fighters[0], &match->fighters[1]);
    versus_fighter_face_opponent(&match->fighters[1], &match->fighters[0]);
    versus_fighter_tick(&match->fighters[0], &input->players[0], controls_enabled);
    versus_fighter_tick(&match->fighters[1], &input->players[1], controls_enabled);
    match_resolve_pushboxes(match);
    match_resolve_attack(match, 0u, input);
    match_resolve_attack(match, 1u, input);

    if (match->round_frames_remaining > 0u) --match->round_frames_remaining;
    if (match->fighters[0].health == 0 || match->fighters[1].health == 0 || match->round_frames_remaining == 0u) match_finish_round(match);
}

void versus_match_render(VersusMatch *match, const UViewport *viewport) {
    if (match == NULL || viewport == NULL) return;
    unsigned_sprite_renderer_draw(&match->fighters[0].sprite, viewport, &match->fighters[0].position);
    unsigned_sprite_renderer_draw(&match->fighters[1].sprite, viewport, &match->fighters[1].position);
}

bool versus_match_finished(const VersusMatch *match) {
    return match != NULL && match->phase == VERSUS_MATCH_OVER;
}
