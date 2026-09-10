# Versus Fighting POC

A small C99 Neo Geo AES/MVS proof of concept showing how to build a local 1-vs-1 fighting game with the existing Unsigned runtime.

The goal of this example is educational: each file has one clear responsibility, generic systems stay in Unsigned, and the example only implements rules that are specific to a versus fighting game.

## What comes from Unsigned

The example intentionally reuses the engine instead of recreating parallel systems:

- `UGameInstance` owns input, actor pools, level runtime, collision, viewport and rendering;
- each fighter is a `UPlayer` backed by `UCharacter -> UActor -> USprite`;
- health is stored as a `UGameplayAttribute` in `UCharacter.attributes`;
- `UStateGraph` drives fighter states and round/match states;
- `unsigned_player_pool_reserve()` owns player/controller routing;
- `ULevelDefinition` represents the fighting arena;
- the level collision pipeline transforms frame hitboxes/hurtboxes and produces `UCollisionHit` pairs;
- `ULevelRenderer` renders the fighters from their normal `UActor` state.

The example therefore adds only fighting-game concepts: movement rules, command history, attacks, pushboxes, hit/block reactions, hitstop and round rules.

## Runtime model

Each fighter follows the normal Unsigned composition:

```text
UPlayer
  -> UCharacter
       -> UActor
            -> USprite
```

`VersusFighter` adds only the state needed by the genre:

```text
VersusFighter
  player / character        generic Unsigned runtime
  attributes                health through UGameplayAttribute
  states                    UStateGraph adapter
  input_buffer              directional command history
  velocity                  fighting movement
  attack                    current move
  stun_frames               hitstun / blockstun
  attack_connected          one hit per move in this POC
```

There is no duplicate position, sprite, health or current-state field. `UActor.position`, `UCharacter.attributes`, `USprite` and `UStateGraph.current` remain the sources of truth.

## Responsibility split

| File | Responsibility |
| --- | --- |
| `main.c` | Minimal Neo Geo entry point. Builds `UNeoGeoRuntimeDefinition` and hands control to Unsigned. |
| `src/versus_game.*` | Application lifecycle. Creates `UGameInstance`, connects fixed storage, handles AES/MVS phases and coordinates match/world ticks. |
| `src/versus_config.h` | Small tunable values: health, timings, stage limits, movement speeds and fixed capacities. |
| `src/versus_arena.*` | The `ULevelDefinition`. Creates/reserves the two players and forwards Unsigned collision hits to the match rules. |
| `src/versus_match.*` | Round and match rules only: intro/fight/outro flow, timer, score, pushbox separation, damage/block decision and hitstop. |
| `src/versus_fighter.*` | Complete behavior of one fighter: state graph, animation selection, command history/QCF recognition, movement, attacks, jump, stun and reactions. |
| `src/versus_state.*` | Tiny common adapter that removes repeated boilerplate for simple enum-like `UStateGraph` graphs. |
| `src/versus_content.*` | Immutable content: animations, frame timings, hitboxes/hurtboxes, palettes and attack tuning. |
| `src/versus_hud.*` | FIX-layer presentation only. It reads match/fighter state and does not own gameplay rules. |

## Recommended reading order

For a beginner, read the example in this order:

1. `main.c` -- see how a Neo Geo application enters Unsigned.
2. `versus_game.c` -- see how one `UGameInstance` is configured and ticked.
3. `versus_arena.c` -- see how a `ULevelDefinition` creates players and receives collision results.
4. `versus_match.c` -- see the rules of one versus match.
5. `versus_fighter.c` -- see one fighter end-to-end: states, animations, input commands, movement and combat reactions.
6. `versus_content.c` -- tune animation frames, collision boxes and attack properties without changing the rules.

This order goes from composition to rules to implementation details.

## Fighting flow

A normal frame during the fight is intentionally straightforward:

```text
Neo Geo runtime polls input
        |
        v
versus_match_update()
  - facing
  - fighter decisions/movement
  - round timer / pushboxes
        |
        v
unsigned_game_instance_tick()
  - player/actor animation tick
  - level collision registration
  - hit detection
        |
        v
VERSUS_ARENA_LEVEL.resolve_hits
        |
        v
versus_match_resolve_hit()
  - block or damage
  - hitstun / blockstun
  - hitstop
        |
        v
unsigned_game_instance_render()
        +
versus_hud_render()
```

During hitstop, the example freezes both match logic and the Unsigned actor/animation world tick. This keeps the visual pause consistent with fighting-game behavior.

## State machines

Two simple state machines use Unsigned `UStateGraph`:

- fighter: `idle`, `walk`, `crouch`, `jump`, `attack`, `block`, `hitstun`, `KO`;
- match: `intro`, `fight`, `outro`, `match over`.

`versus_state.*` does not replace `UStateGraph`. It only owns the repetitive fixed node/transition storage for this common pattern: event `N` enters state `N`.

The active state is always derived from `UStateGraph.current`; there is no second enum field that could become desynchronized.

For readability, all fighter-specific state handling remains in `versus_fighter.c` instead of being split across several tiny implementation files. The file is organized into sections: state/animation, command input, lifecycle, per-frame behavior and combat reactions.

## Content-driven attacks

Attack behavior is described in `VersusAttackDefinition` rather than spread through match logic. Each attack defines:

- animation;
- damage;
- pushback;
- hitstun;
- blockstun;
- hitstop on hit;
- hitstop on block.

The active frames themselves remain authored in the corresponding `UFrame` arrays through their hitboxes. This keeps move data separate from move execution.

The QCF command buffer is intentionally custom. Unsigned input/ability bindings are appropriate for ordinary button triggers, while a fighting-game directional sequence needs ordered input history. The parser remains a small private section inside `versus_fighter.c`, keeping the whole fighter behavior visible in one place for a beginner.

## Controls

- Left / Right: walk
- Down: crouch
- Up: jump
- Hold away from the opponent: block
- A: light attack
- B: heavy attack
- Down, down-forward, forward + A: special attack

A recognized QCF is consumed when the special starts, so an old directional sequence cannot accidentally trigger another special on a later A press.

## Spritesheets

The teaching art comes from The Spriters Resource, using Kyo and Iori from *The King of Fighters R-2*. Source attribution is kept in `assets/source/README.md`.

The automatic asset pipeline downloads the source sheets, detects and normalizes frames to 64x64 cells, builds a shared indexed atlas, then generates `fighters.c1` and `fighters.c2` with ngdevkit `tiletool.py`.

The importer is intentionally simple and educational. Asset extraction is independent from the runtime architecture, so the exact animation mapping can be refined without changing gameplay code.

## Build

From the repository root:

```sh
bash scripts/setup-deps.sh
cd versus-fighting
make gngeo-aes
# or
make gngeo-mvs
```

Python 3 and ImageMagick (`magick`) are required by the automatic graphics pipeline in addition to the normal ngdevkit toolchain.

To force a complete regeneration:

```sh
make distclean
make gngeo-aes
```
