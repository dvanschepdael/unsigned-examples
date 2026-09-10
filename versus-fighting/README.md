# Versus Fighting POC

A C99 Neo Geo AES/MVS proof of concept showing how to build a local 1-vs-1 fighting game **with the existing Unsigned runtime**, instead of recreating actor, level, state, collision and rendering systems beside the engine.

## What the POC demonstrates

- two local `UPlayer` instances backed by `UCharacter` / `UActor`;
- a real `ULevelDefinition` for the fighting arena;
- `UGameInstance` as the top-level gameplay/runtime composition root;
- `UStateGraph` for fighter states and match/round flow;
- Unsigned player pools for ownership and controller routing;
- Unsigned level collision registration and hit detection;
- Unsigned level renderer and viewport for actor rendering;
- best-of-three rounds and a 60-second timer;
- automatic facing, walking, crouching and jumping;
- light attack, heavy attack and quarter-circle-forward + A special;
- frame-authored hitboxes and hurtboxes;
- hitstun, blockstun, hitstop and pushback;
- health HUD.

## Architecture

The example deliberately separates reusable engine responsibilities from versus-specific rules.

`VersusGame` owns a `UGameInstance` plus its caller-owned fixed storage. Unsigned therefore owns the input manager, actor pools, level runtime, collision manager, viewport and level renderer.

Each `VersusFighter` is composed as:

```text
UPlayer
  -> UCharacter
       -> UActor
            -> USprite
```

The fighter-specific structure only adds data that is specific to a fighting game: command history, velocity, health, current attack, hitstun and its `UStateGraph`.

The arena is a `ULevelDefinition`. Its `load` callback creates/reserves both `UPlayer` instances and its `resolve_hits` callback interprets Unsigned's generic `UCollisionHit` results as fighting-game damage/block reactions.

Both fighter flow (`idle`, `walk`, `crouch`, `jump`, `attack`, `block`, `hitstun`, `KO`) and match flow (`intro`, `fight`, `outro`, `match over`) use `UStateGraph` rather than hand-maintained switch-only state machines.

`UActor.position` remains the gameplay/physics origin at the fighter's feet. The visual `-32,-64` alignment is stored in `USprite.offset`, so rendering uses the engine's normal `ULevelRenderer` without special-case coordinate conversion.

## Source layout

- `main.c`: adapts the example to `UNeoGeoRuntimeDefinition`.
- `src/versus_game.*`: creates/configures `UGameInstance` and integrates AES/MVS phases.
- `src/versus_match.*`: `ULevelDefinition`, match `UStateGraph`, round rules and interpretation of level collision hits.
- `src/versus_fighter.*`: `UPlayer`/`UCharacter` composition, fighter `UStateGraph`, movement and command buffer.
- `src/versus_content.*`: immutable animation/frame/hitbox content.
- `src/versus_hud.*`: FIX-layer HUD only.
- `scripts/prepare-fighting-assets.py`: downloads/normalizes teaching spritesheets and generates Neo Geo C-ROM data.

## Controls

- Left / Right: walk
- Down: crouch
- Up: jump
- Hold away from the opponent: block
- A: light attack
- B: heavy attack
- Down, down-forward, forward + A: special attack

## Spritesheets

The teaching art comes from The Spriters Resource, using Kyo and Iori from *The King of Fighters R-2*. Source attribution is kept in `assets/source/README.md`.

The automatic asset pipeline downloads the source sheets, detects/normalizes frames to 64x64 cells, builds a shared indexed atlas, then generates `fighters.c1` / `fighters.c2` with ngdevkit `tiletool.py`.

The extraction is intentionally a simple educational importer. It is independent from the runtime architecture and the exact semantic animation mapping can be refined separately.

## Build

From the repository root:

```sh
bash scripts/setup-deps.sh
cd versus-fighting
make gngeo-aes
# or
make gngeo-mvs
```

Requirements for automatic graphics generation are Python 3 and ImageMagick (`magick`) in addition to the normal ngdevkit toolchain.

To force regeneration:

```sh
make distclean
make gngeo-aes
```
