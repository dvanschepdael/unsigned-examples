# Versus Fighting POC

A small C99 Neo Geo AES/MVS proof of concept showing how to structure a local 1-vs-1 fighting game with Unsigned. It deliberately favors readable data flow over feature count.

## What the POC demonstrates

- two local controllers;
- best-of-three rounds and a 60-second round timer;
- intro / fight / KO / round-out / match-over flow;
- automatic facing;
- walking, crouching and jumping;
- light attack, heavy attack and quarter-circle-forward + A special;
- frame-authored hitboxes and hurtboxes;
- hitstun, blockstun, hitstop and pushback;
- simple pushbox separation;
- health HUD;
- separation between platform lifecycle, match rules, fighter state and content data.

The architecture follows the responsibility split used by traditional 2D fighters: the match owns round rules, a fighter owns one character's state, animation data owns active hurt/hit geometry, and the Neo Geo adapter owns BIOS/runtime lifecycle.

## Source layout

- `main.c`: BIOS/Unsigned composition root only.
- `src/versus_game.*`: Neo Geo runtime adapter and screen phase integration.
- `src/versus_match.*`: versus rules, hit resolution, round/match state.
- `src/versus_fighter.*`: character state machine, movement, input buffer and move execution.
- `src/versus_content.*`: immutable animation, timing, hitbox/hurtbox and sprite layout data.
- `src/versus_hud.*`: FIX-layer presentation.
- `scripts/prepare-fighting-assets.py`: downloads and normalizes the teaching spritesheets, then generates Neo Geo C-ROM data.
- `scripts/prepare-fighting-assets.sh`: small shell entry point used by Make.

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

The asset pipeline is automatic. It:

1. downloads the Kyo and Iori source sheets from their Spriters Resource asset pages;
2. verifies the expected source dimensions;
3. removes the sheet background and detects sprite-like connected regions;
4. selects 29 regions per fighter in deterministic row-major order;
5. normalizes every frame to a 64x64 Neo Geo-friendly cell;
6. builds one shared 16-color indexed atlas;
7. generates `fighters.c1`, `fighters.c2` with ngdevkit `tiletool.py`;
8. generates `fighters.pal` with ngdevkit `paltool.py`.

Generated files are placed under `assets/generated/` and are intentionally not source-authored files.

You can run the asset step explicitly:

```sh
make assets
```

Normal builds also depend on this step, so `make gngeo-aes` or `make gngeo-mvs` will generate the fighter graphics automatically when needed.

The extraction is deliberately simple and educational rather than a production-grade semantic animation importer. The generated frame sequence can be inspected under `assets/generated/frames/p1/` and `assets/generated/frames/p2/` if you want to refine the exact animation mapping later.

## Build

From the repository root, install the pinned dependencies once:

```sh
bash scripts/setup-deps.sh
cd versus-fighting
make gngeo-aes
# or
make gngeo-mvs
```

Requirements for the automatic graphics pipeline are Python 3 and ImageMagick (`magick`) in addition to the normal ngdevkit toolchain.

To force a complete regeneration of SDK outputs and downloaded/generated graphics:

```sh
make distclean
make gngeo-aes
```

The project starts from the `unsigned-template` architecture and keeps its pinned Unsigned/ngdevkit versions and AES/MVS runtime lifecycle.
