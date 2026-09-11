# Unsigned UI examples

Small Neo Geo AES/MVS project showing the public UI API from **Unsigned** one concept at a time.
The project is intentionally organized for beginners: platform/runtime code is centralized, common
UI setup is shared, and every demonstration lives in one `src/levels/level_*.c` file.

## What is demonstrated

| Example | Unsigned API |
| --- | --- |
| 01 Label | `UUILabel`, `unsigned_ui_label_set_text()` |
| 02 Blink label | `UUIBlinkLabel`, `unsigned_ui_blink_label_tick()` |
| 03 Panel | `UUIPanel` |
| 04 Button | `UUIButton`, callback, menu focus |
| 05 Progress bar | `UUIProgressBar`, bounded values |
| 06 Selector | `UUISelector`, option callback, menu focus |
| 07 Image | `UIImage`, `UUIAssetId`, `unsigned_ui_image_set_asset()` |
| 08 Linear layout | `unsigned_ui_layout_linear()` |
| 09 Menu | `UUIMenu`, focus navigation, disabled items |
| 10 Page stack | `UUIContext`, `UUIPage`, overlay push/pop |

`UUIElement`, `UUIScreen`, `UUIRenderer`, `UUITheme`, `UUIInput` and `UUIContext` are infrastructure
used by all examples, so they are kept in the shared layer instead of duplicated as artificial
standalone examples.

## Controls

- **C / D**: previous / next example
- **Up / Down**: move menu focus
- **Left / Right**: change a selector/value where the example supports it
- **A**: confirm/action
- **B**: cancel/close an overlay

Each example also displays its relevant controls on screen.

## Project structure

```text
ui/
├── main.c                 Neo Geo runtime entry points only
├── Makefile               ROM build and emulator targets
└── src/
    ├── ui_app.*           application lifecycle and example switching
    ├── ui_common.*        shared page, input and renderer helpers
    ├── ui_levels.*        example registry
    └── levels/
        └── level_*.c      one self-contained example per concept
```

This keeps responsibilities separate:

1. `main.c` knows how to start the Unsigned Neo Geo runtime.
2. `ui_app.c` owns global input/UI context and changes examples.
3. `ui_common.c` contains only reusable presentation/input glue.
4. A `level_*.c` file owns only the state and behavior required by that demonstration.

The gameplay `ULevel` type is deliberately not used as a UI container. `ULevel` owns gameplay
concerns such as actors, collision and spawns; `UUIPage`/`UUIContext` are the proper Unsigned
abstractions for UI screen lifecycle and overlays.

## Neo Geo rendering

Unsigned's Neo Geo UI backend already renders `UUILabel` and `UUISelector` on the FIX layer. The
backend intentionally leaves panel, button, progress-bar and image drawing callbacks to the game.
`ui_common.c` fills only those missing callbacks while keeping the existing Unsigned label/selector
renderer intact.

To keep this teaching project asset-free, the Image example renders its `UUIAssetId` as a FIX-layer
placeholder. In a game, replace only the image draw callback with the project's sprite/asset lookup;
the `UIImage` example code does not need to change.

## Build

From the repository root:

```sh
./scripts/setup-deps.sh
cd ui
make -j2
```

Run with GnGeo:

```sh
make gngeo-aes
# or
make gngeo-mvs
```

The Makefile follows the same pinned ngdevkit/Unsigned setup as the other examples in this repository.

## Reading order

For a first pass, read these files in order:

1. `src/levels/level_label.c`
2. `src/levels/level_button.c`
3. `src/levels/level_selector.c`
4. `src/levels/level_menu.c`
5. `src/levels/level_pages.c`
6. `src/ui_common.c`
7. `src/ui_app.c`

## Add another example

1. Create `src/levels/level_my_widget.c`.
2. Keep its mutable state in that file.
3. Implement a small `enter` function and an optional `tick` function.
4. Export one `const UIExampleLevel` descriptor.
5. Add the descriptor to `src/ui_levels.c`.

There is no extra switch statement in the runtime: the registry is the single list of available
examples.
