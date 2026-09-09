# unsigned-examples

Educational Neo Geo AES/MVS examples for the Unsigned engine.

Each genre lives in its own directory and keeps gameplay-specific code local while sharing pinned dependencies under `external/`.

## Examples

- [`versus-fighting/`](versus-fighting/) - local 1-vs-1 fighting game POC with frame data, command buffering, blocking, hitstop, rounds and match flow.

## Setup

```sh
git clone https://github.com/dvanschepdael/unsigned-examples.git
cd unsigned-examples
./scripts/setup-deps.sh
cd versus-fighting
make -j2
```
