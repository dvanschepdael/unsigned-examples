#include "versus_match.h"

static VersusRoundPhase phase_from_node(const VersusMatch *match, const UStateGraphNode *node) {
    for (u8 i = 0u; i < VERSUS_ROUND_PHASE_COUNT; ++i) {
        if (node == &match->state_nodes[i]) return (VersusRoundPhase)i;
    }
    return VERSUS_ROUND_INTRO;
}

VersusRoundPhase versus_match_phase(const VersusMatch *match) {
    if (match == NULL) return VERSUS_ROUND_INTRO;
    return phase_from_node(match, versus_state_machine_current(&match->states));
}

static void match_state_enter(UStateGraph *graph, void *context) {
    VersusMatch *match = context;
    if (match == NULL || graph == NULL) return;

    switch (phase_from_node(match, graph->current)) {
        case VERSUS_ROUND_INTRO:
            match->phase_frames = (u16)(match->refresh_rate * VERSUS_INTRO_SECONDS);
            break;
        case VERSUS_ROUND_OUTRO:
            match->phase_frames = (u16)(match->refresh_rate * VERSUS_OUTRO_SECONDS);
            break;
        default:
            match->phase_frames = 0u;
            break;
    }
}

static void set_phase(VersusMatch *match, VersusRoundPhase phase) {
    if (match != NULL && phase < VERSUS_ROUND_PHASE_COUNT) {
        versus_state_machine_set(&match->states, (u8)phase);
    }
}

static void begin_round(VersusMatch *match) {
    versus_fighter_reset(&match->fighters[0], (Vec2){ VERSUS_PLAYER_1_START_X, VERSUS_GROUND_Y }, true);
    versus_fighter_reset(&match->fighters[1], (Vec2){ VERSUS_PLAYER_2_START_X, VERSUS_GROUND_Y }, false);

    match->round_frames_remaining = (u16)(match->refresh_rate * VERSUS_ROUND_SECONDS);
    match->hitstop_frames = 0u;
    match->winner = 0xffu;
    ++match->round_number;
    set_phase(match, VERSUS_ROUND_INTRO);
}

bool versus_match_init(VersusMatch *match, u16 refresh_rate) {
    if (match == NULL || refresh_rate == 0u) return false;

    *match = (VersusMatch){
        .refresh_rate = refresh_rate,
        .winner = 0xffu,
    };

    return versus_state_machine_init(
        &match->states,
        match->state_nodes,
        match->state_transitions,
        VERSUS_ROUND_PHASE_COUNT,
        VERSUS_ROUND_INTRO,
        match_state_enter,
        match
    );
}

void versus_match_start(VersusMatch *match) {
    if (match == NULL) return;
    for (u8 i = 0u; i < VERSUS_PLAYER_COUNT; ++i) match->rounds_won[i] = 0u;
    match->round_number = 0u;
    begin_round(match);
}

static void resolve_pushboxes(VersusMatch *match) {
    UActor *a = versus_fighter_actor(&match->fighters[0]);
    UActor *b = versus_fighter_actor(&match->fighters[1]);
    UActor *left = a;
    UActor *right = b;

    if (a == NULL || b == NULL) return;
    if (a->position.x > b->position.x) {
        left = b;
        right = a;
    }

    const s16 distance = (s16)(right->position.x - left->position.x);
    if (distance >= VERSUS_PUSHBOX_DISTANCE) return;

    const s16 correction = (s16)((VERSUS_PUSHBOX_DISTANCE - distance + 1) / 2);
    left->position.x = (s16)(left->position.x - correction);
    right->position.x = (s16)(right->position.x + correction);
}

static void finish_round(VersusMatch *match) {
    const s16 p1_health = versus_fighter_health(&match->fighters[0]);
    const s16 p2_health = versus_fighter_health(&match->fighters[1]);

    match->winner = p1_health == p2_health ? 0xffu : (p1_health > p2_health ? 0u : 1u);
    if (match->winner < VERSUS_PLAYER_COUNT) ++match->rounds_won[match->winner];
    set_phase(match, VERSUS_ROUND_OUTRO);
}

static VersusFighter *fighter_from_actor(VersusMatch *match, UActor *actor) {
    if (match == NULL || actor == NULL) return NULL;
    for (u8 i = 0u; i < VERSUS_PLAYER_COUNT; ++i) {
        if (actor == versus_fighter_actor(&match->fighters[i])) return &match->fighters[i];
    }
    return NULL;
}

void versus_match_resolve_hit(VersusMatch *match, UActor *attacker_actor, UActor *target_actor) {
    if (match == NULL || match->frame_input == NULL || versus_match_phase(match) != VERSUS_ROUND_FIGHT) return;

    VersusFighter *attacker = fighter_from_actor(match, attacker_actor);
    VersusFighter *defender = fighter_from_actor(match, target_actor);
    if (attacker == NULL || defender == NULL || attacker == defender || attacker->attack_connected) return;

    const VersusAttackDefinition *attack = versus_content_attack(attacker->attack);
    if (attack == NULL) return;

    attacker->attack_connected = true;
    const s16 direction = attacker->character.facing_right ? 1 : -1;
    const UInputController *defender_input = &match->frame_input->players[defender->player.controller_index];

    if (versus_fighter_is_blocking(defender, defender_input)) {
        versus_fighter_apply_block(defender, attack, direction);
        match->hitstop_frames = attack->block_hitstop_frames;
    } else {
        versus_fighter_apply_hit(defender, attack, direction);
        match->hitstop_frames = attack->hitstop_frames;
    }
}

void versus_match_update(VersusMatch *match, const UInputManager *input) {
    if (match == NULL || input == NULL) return;
    match->frame_input = input;

    if (match->hitstop_frames > 0u) {
        --match->hitstop_frames;
        return;
    }

    versus_fighter_face_opponent(&match->fighters[0], &match->fighters[1]);
    versus_fighter_face_opponent(&match->fighters[1], &match->fighters[0]);

    const VersusRoundPhase phase = versus_match_phase(match);
    const bool controls_enabled = phase == VERSUS_ROUND_FIGHT;

    for (u8 i = 0u; i < VERSUS_PLAYER_COUNT; ++i) {
        versus_fighter_update(&match->fighters[i], &input->players[i], controls_enabled);
    }

    if (phase == VERSUS_ROUND_INTRO || phase == VERSUS_ROUND_OUTRO) {
        if (match->phase_frames > 0u) --match->phase_frames;
        if (match->phase_frames != 0u) return;

        if (phase == VERSUS_ROUND_INTRO) {
            set_phase(match, VERSUS_ROUND_FIGHT);
        } else if (match->rounds_won[0] >= VERSUS_ROUNDS_TO_WIN || match->rounds_won[1] >= VERSUS_ROUNDS_TO_WIN) {
            set_phase(match, VERSUS_MATCH_OVER);
        } else {
            begin_round(match);
        }
        return;
    }

    if (phase != VERSUS_ROUND_FIGHT) return;

    resolve_pushboxes(match);
    if (match->round_frames_remaining > 0u) --match->round_frames_remaining;

    if (versus_fighter_health(&match->fighters[0]) == 0 ||
        versus_fighter_health(&match->fighters[1]) == 0 ||
        match->round_frames_remaining == 0u) {
        finish_round(match);
    }
}

bool versus_match_finished(const VersusMatch *match) {
    return match != NULL && versus_match_phase(match) == VERSUS_MATCH_OVER;
}
