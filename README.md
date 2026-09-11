# unsigned-examples

Educational Neo Geo AES/MVS examples for the Unsigned engine.

Each genre or subsystem lives in its own directory and keeps example-specific code local while sharing pinned dependencies under `external/`.

## Examples

- [`versus-fighting/`](versus-fighting/) - local 1-vs-1 fighting game POC with frame data, command buffering, blocking, hitstop, rounds and match flow.
- [`ui/`](ui/) - beginner-friendly UI component gallery with one demonstration level per widget/concept, shared runtime code, menu navigation and page overlays.

## Setup

```sh
git clone https://github.com/dvanschepdael/unsigned-examples.git
cd unsigned-examples
./scripts/setup-deps.sh
cd ui
make -j2
```

Each example directory has its own README with controls and architecture notes.

## Run with MAME

From either `ui/` or `versus-fighting/`, using an MSYS2 UCRT64 shell on Windows:

```sh
make mame-aes
# or
make mame-mvs
```

`make mame` defaults to AES. These targets build the ROMs and development BIOS
before starting MAME in a window. MAME must be installed separately and available
on `PATH`. To use another executable, create `config.local.mk` in the example directory:

```make
MAME := C:/Games/MAME/mame.exe
```

Optional emulator arguments can be set with `MAME_FLAGS` in the same file or on
the command line, for example `make mame-aes MAME_FLAGS="-window -skip_gameinfo"`.
MAME settings and NVRAM are stored under the example's `build/mame/` directory.
The generated cartridges are `build/rom/unsigned_ui.zip` and
`build/rom/unsigned_versus.zip`; their short names use underscores for MAME compatibility.
