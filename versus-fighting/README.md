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
- `scripts/prepare-fighting-assets.sh`: packs normalized source frames into Neo Geo C-ROM data.

## Controls

- Left / Right: walk
- Down: crouch
- Up: jump
- Hold away from the opponent: block
- A: light attack
- B: heavy attack
- Down, down-forward, forward + A: special attack

## Spritesheets

The reference art comes from The Spriters Resource, using Kyo and Iori from *The King of Fighters R-2*. This is an educational example and the source attribution is kept in `assets/source/README.md`.

After preparing `assets/source/p1/000.png..028.png` and `assets/source/p2/000.png..028.png`:

```sh
./scripts/prepare-fighting-assets.sh
make -j2
```

The script follows ngdevkit's sprite workflow: normalize frames, combine them into a shared-palette atlas, run `tiletool.py --sprite -c`, and generate palette data with `paltool.py`.

Without generated fighter C-ROM files the project still builds, but the fighters are invisible; the gameplay/HUD logic remains usable for development and tests.

## Build

From the repository root, install the pinned dependencies once:

```sh
./scripts/setup-deps.sh
cd versus-fighting
make -j2
make gngeo-aes
# or
make gngeo-mvs
```

The project starts from the `unsigned-template` architecture and keeps its pinned Unsigned/ngdevkit versions and AES/MVS runtime lifecycle.
