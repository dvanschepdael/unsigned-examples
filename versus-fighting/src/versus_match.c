#include "versus_match.h"

#include "actor/player_pool.h"
#include "level/level_actor.h"

#define ROUND_SECONDS 60u
#define INTRO_SECONDS 2u
#define OUTRO_SECONDS 2u
#define ROUNDS_TO_WIN 2u
#define MIN_SEPARATION 24

static VersusRoundPhase match_graph_phase(const VersusMatch *match, const UStateGraphNode *node) {
    if (node == NULL) return VERSUS_ROUND_INTRO;
    for (u8 i = 0u; i < VERSUS_ROUND_PHASE_COUNT; ++i) {
        if (node == &match->state_nodes[i]) return (VersusRoundPhase)i;
    }
    return VERSUS_ROUND_INTRO;
}

static void match_state_enter(UStateGraph *graph, void *context) {
    VersusMatch *match = context;
    if (match == NULL || graph == NULL) return;
    match->phase = match_graph_phase(match, graph->current);
    if (match->phase == VERSUS_ROUND_INTRO) match->phase_frames = (u16)(match->refresh_rate * INTRO_SECONDS);
    else if (match->phase == VERSUS_ROUND_OUTRO) match->phase_frames = (u16)(match->refresh_rate * OUTRO_SECONDS);
    else match->phase_frames = 0u;
}

static bool match_state_graph_init(VersusMatch *match) {
    match->state_root = (UStateGraphNode){ 0 };
    match->state_children = (UStateGraphNodeContainer){
        .count = VERSUS_ROUND_PHASE_COUNT,
        .capacity = VERSUS_ROUND_PHASE_COUNT,
        .instances = match->state_nodes,
    };
    match->state_event_container = (UStateGraphTransitionContainer){
        .count = VERSUS_ROUND_PHASE_COUNT,
        .capacity = VERSUS_ROUND_PHASE_COUNT,
        .instances = match->state_events,
    };
    match->state_root.edges = &match->state_children;
    match->state_root.transitions[U_TRANSITION_ON_EVENT] = &match->state_event_container;

    for (u8 i = 0u; i < VERSUS_ROUND_PHASE_COUNT; ++i) {
        match->state_nodes[i] = (UStateGraphNode){ .parent = &match->state_root, .enter = match_state_enter };
        match->state_events[i] = (UStateGraphTransition){ .target = &match->state_nodes[i] };
    }
    return unsigned_state_graph_init(&match->state_graph, &match->state_root, &match->state_nodes[VERSUS_ROUND_INTRO], match);
}

static void match_set_phase(VersusMatch *match, VersusRoundPhase phase) {
    if (match == NULL || phase >= VERSUS_ROUND_PHASE_COUNT) return;
    unsigned_state_graph_send_event(&match->state_graph, (UEvent)phase);
}

static void match_begin_round(VersusMatch *match) {
    versus_fighter_reset(&match->fighters[0], 92, 184, true);
    versus_fighter_reset(&match->fighters[1], 228, 184, false);
    match->round_frames_remaining = (u16)(match->refresh_rate * ROUND_SECONDS);
    match->hitstop_frames = 0u;
    ++match->round_number;
    match_set_phase(match, VERSUS_ROUND_INTRO);
}

static bool versus_arena_load(ULevel *level, const ULevelDefinition *definition, void *context) {
    VersusMatch *match = context;
    UPoolInstanceContainer *players;
    (void)definition;
    if (level == NULL || match == NULL) return false;
    players = unsigned_level_player_pool(level);
    if (players == NULL) return false;

    if (!versus_fighter_init(&match->fighters[0], &VERSUS_FIGHTER_P1_SPRITE, 1u, 0u, 92, 184, true)) return false;
    if (!versus_fighter_init(&match->fighters[1], &VERSUS_FIGHTER_P2_SPRITE, 5u, 1u, 228, 184, false)) return false;
    if (unsigned_player_pool_reserve(players, &match->fighters[0].player, 0u) == NULL) return false;
    if (unsigned_player_pool_reserve(players, &match->fighters[1].player, 1u) == NULL) return false;
    return true;
}

static VersusFighter *match_fighter_from_actor(VersusMatch *match, UActor *actor) {
    if (actor == versus_fighter_actor(&match->fighters[0])) return &match->fighters[0];
    if (actor == versus_fighter_actor(&match->fighters[1])) return &match->fighters[1];
    return NULL;
}

static s16 attack_damage(VersusAttackKind attack) {
    if (attack == VERSUS_ATTACK_LIGHT) return 8;
    if (attack == VERSUS_ATTACK_HEAVY) return 14;
    if (attack == VERSUS_ATTACK_SPECIAL) return 20;
    return 0;
}

static u8 attack_hitstun(VersusAttackKind attack) {
    if (attack == VERSUS_ATTACK_LIGHT) return 10u;
    if (attack == VERSUS_ATTACK_HEAVY) return 16u;
    if (attack == VERSUS_ATTACK_SPECIAL) return 22u;
    return 0u;
}

static void versus_arena_resolve_hits(ULevel *level, const ULevelDefinition *definition, void *context) {
    VersusMatch *match = context;
    (void)definition;
    if (level == NULL || match == NULL || match->frame_input == NULL || match->phase != VERSUS_ROUND_FIGHT) return;

    for (u16 i = 0u; i < level->collision.hits.count; ++i) {
        UCollisionHit *hit = &level->collision.hits.instances[i];
        VersusFighter *attacker = match_fighter_from_actor(match, hit->attacker);
        VersusFighter *defender = match_fighter_from_actor(match, hit->target);
        s16 direction;
        if (attacker == NULL || defender == NULL || attacker == defender || attacker->attack == VERSUS_ATTACK_NONE || attacker->attack_connected) continue;

        attacker->attack_connected = 1u;
        direction = attacker->character.facing_right ? 2 : -2;
        if (versus_fighter_is_blocking(defender, &match->frame_input->players[defender->player.controller_index])) {
            versus_fighter_apply_block(defender, 7u, direction);
            match->hitstop_frames = 4u;
        } else {
            versus_fighter_apply_hit(defender, attack_damage(attacker->attack), attack_hitstun(attacker->attack), direction);
            match->hitstop_frames = 6u;
        }
    }
}

const ULevelDefinition VERSUS_ARENA_LEVEL = {
    .actor_order = U_ACTOR_ORDER_Y_X_STABLE,
    .backdrop_color = 0x0000,
    .load = versus_arena_load,
    .resolve_hits = versus_arena_resolve_hits,
};

bool versus_match_init(VersusMatch *match, u16 refresh_rate) {
    if (match == NULL || refresh_rate == 0u) return false;
    *match = (VersusMatch){ .refresh_rate = refresh_rate, .winner = 0xffu };
    return match_state_graph_init(match);
}

void versus_match_start(VersusMatch *match) {
    if (match == NULL) return;
    match->rounds_won[0] = 0u;
    match->rounds_won[1] = 0u;
    match->round_number = 0u;
    match->winner = 0xffu;
    match_begin_round(match);
}

static void match_resolve_pushboxes(VersusMatch *match) {
    VersusFighter *left = &match->fighters[0];
    VersusFighter *right = &match->fighters[1];
    UActor *left_actor = versus_fighter_actor(left);
    UActor *right_actor = versus_fighter_actor(right);
    if (left_actor->position.x > right_actor->position.x) {
        VersusFighter *swap = left;
        left = right;
        right = swap;
        left_actor = versus_fighter_actor(left);
        right_actor = versus_fighter_actor(right);
    }
    s16 distance = (s16)(right_actor->position.x - left_actor->position.x);
    if (distance < MIN_SEPARATION) {
        s16 correction = (s16)((MIN_SEPARATION - distance + 1) / 2);
        left_actor->position.x = (s16)(left_actor->position.x - correction);
        right_actor->position.x = (s16)(right_actor->position.x + correction);
    }
}

static void match_finish_round(VersusMatch *match) {
    s16 p1 = match->fighters[0].health;
    s16 p2 = match->fighters[1].health;
    match->winner = p1 == p2 ? 0xffu : (p1 > p2 ? 0u : 1u);
    if (match->winner < 2u) ++match->rounds_won[match->winner];
    match_set_phase(match, VERSUS_ROUND_OUTRO);
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

    if (match->phase == VERSUS_ROUND_INTRO) {
        versus_fighter_update(&match->fighters[0], &input->players[0], false);
        versus_fighter_update(&match->fighters[1], &input->players[1], false);
        if (match->phase_frames > 0u) --match->phase_frames;
        if (match->phase_frames == 0u) match_set_phase(match, VERSUS_ROUND_FIGHT);
        return;
    }

    if (match->phase == VERSUS_ROUND_OUTRO) {
        versus_fighter_update(&match->fighters[0], &input->players[0], false);
        versus_fighter_update(&match->fighters[1], &input->players[1], false);
        if (match->phase_frames > 0u) --match->phase_frames;
        if (match->phase_frames == 0u) {
            if (match->rounds_won[0] >= ROUNDS_TO_WIN || match->rounds_won[1] >= ROUNDS_TO_WIN) match_set_phase(match, VERSUS_MATCH_OVER);
            else match_begin_round(match);
        }
        return;
    }

    if (match->phase == VERSUS_MATCH_OVER) return;

    versus_fighter_update(&match->fighters[0], &input->players[0], true);
    versus_fighter_update(&match->fighters[1], &input->players[1], true);
    match_resolve_pushboxes(match);
    if (match->round_frames_remaining > 0u) --match->round_frames_remaining;
    if (match->fighters[0].health == 0 || match->fighters[1].health == 0 || match->round_frames_remaining == 0u) match_finish_round(match);
    unsigned_state_graph_tick(&match->state_graph);
}

bool versus_match_finished(const VersusMatch *match) {
    return match != NULL && match->phase == VERSUS_MATCH_OVER;
}
