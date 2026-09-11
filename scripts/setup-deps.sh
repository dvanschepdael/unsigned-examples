#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EXTERNAL="$ROOT/external"
mkdir -p "$EXTERNAL"

clone_pinned() {
    name=$1
    url=$2
    commit=$3
    dir="$EXTERNAL/$name"
    if [ ! -d "$dir/.git" ]; then
        git clone "$url" "$dir"
    fi
    git -C "$dir" fetch --all --tags
    git -C "$dir" checkout --detach "$commit"
}

clone_pinned ngdevkit https://github.com/dciabrin/ngdevkit.git b36a345dd65097040d48933408586e0a9d7c764b
clone_pinned ngdevkit-examples https://github.com/dciabrin/ngdevkit-examples.git 60f1bd113471ade1a1850e0dca945cffeef38231
clone_pinned unsigned https://github.com/dvanschepdael/unsigned.git 529cc7013f059304b76e76b41b938b50dae22694
