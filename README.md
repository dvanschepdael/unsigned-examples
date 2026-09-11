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
