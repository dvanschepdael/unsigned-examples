#!/usr/bin/env python3
import argparse
import html as html_module
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

ASSETS = (
    {
        "player": "p1",
        "name": "Kyo",
        "page_url": "https://www.spriters-resource.com/neo_geo_pocket/thekingoffightersr2/asset/572882/",
        "size": (792, 469),
    },
    {
        "player": "p2",
        "name": "Iori",
        "page_url": "https://www.spriters-resource.com/neo_geo_pocket/thekingoffightersr2/asset/572880/",
        "size": (796, 583),
    },
)

FRAME_COUNT = 29
FRAME_SIZE = 64
USER_AGENT = "unsigned-examples educational asset builder/1.0 (+https://github.com/dvanschepdael/unsigned-examples)"


def run(command, capture=False):
    options = {"check": True, "text": True}
    if capture:
        options.update(stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    result = subprocess.run(command, **options)
    return result.stdout if capture else ""


def find_magick():
    candidate = os.environ.get("CONVERT", "magick")
    if shutil.which(candidate):
        return candidate
    if candidate == "magick" and shutil.which("convert"):
        return "convert"
    raise SystemExit("ImageMagick is required (expected 'magick', or set CONVERT).")


def request_bytes(url, referer=None):
    headers = {
        "User-Agent": USER_AGENT,
        "Accept": "text/html,image/png,image/*;q=0.9,*/*;q=0.8",
    }
    if referer:
        headers["Referer"] = referer
    request = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(request, timeout=30) as response:
        return response.read(), response.geturl(), response.headers.get_content_type()


def candidate_image_urls(page_html, page_url):
    text = html_module.unescape(page_html)
    patterns = (
        r'(?P<url>https?://[^\"\'<>\s]+/(?:media/assets|resources/sheets)/[^\"\'<>\s]+?\.png(?:\?[^\"\'<>\s]*)?)',
        r'[\"\'](?P<url>/(?:media/assets|resources/sheets)/[^\"\']+?\.png(?:\?[^\"\']*)?)[\"\']',
    )
    urls = []
    for pattern in patterns:
        for match in re.finditer(pattern, text, flags=re.IGNORECASE):
            url = urllib.parse.urljoin(page_url, match.group("url"))
            if url not in urls:
                urls.append(url)
    return urls


def image_size(magick, path):
    result = run([magick, str(path), "-format", "%w %h", "info:"], capture=True).strip()
    width, height = result.split()
    return int(width), int(height)


def download_sheet(magick, asset, raw_dir):
    output = raw_dir / f"{asset['player']}.png"
    if output.exists() and image_size(magick, output) == asset["size"]:
        print(f"using cached {asset['name']} sheet: {output}")
        return output

    page, final_url, content_type = request_bytes(asset["page_url"])
    candidates = [final_url] if content_type.startswith("image/") else candidate_image_urls(
        page.decode("utf-8", errors="replace"), final_url
    )
    if not candidates:
        raise RuntimeError(f"could not find the PNG URL on {asset['page_url']}")

    with tempfile.TemporaryDirectory() as temporary:
        for index, candidate in enumerate(candidates):
            try:
                data, _, candidate_type = request_bytes(candidate, asset["page_url"])
            except Exception as error:
                print(f"warning: failed candidate {candidate}: {error}", file=sys.stderr)
                continue
            if not candidate_type.startswith("image/"):
                continue
            temporary_image = Path(temporary) / f"candidate-{index}.png"
            temporary_image.write_bytes(data)
            try:
                size = image_size(magick, temporary_image)
            except subprocess.CalledProcessError:
                continue
            if size == asset["size"]:
                output.write_bytes(data)
                print(f"downloaded {asset['name']} sheet: {candidate}")
                return output

    raise RuntimeError(
        f"PNG candidates for {asset['name']} did not match the expected "
        f"{asset['size'][0]}x{asset['size'][1]} sheet; the source page may have changed"
    )


COMPONENT_RE = re.compile(
    r"^\s*\d+:\s+(\d+)x(\d+)\+(\d+)\+(\d+)\s+[^\s]+\s+(\d+)\s+(.+)$"
)


def sort_row_major(components):
    rows = []
    for component in sorted(components, key=lambda item: (item["y"] + item["h"] // 2, item["x"])):
        center_y = component["y"] + component["h"] / 2.0
        selected_row = None
        selected_distance = 9999.0
        for row in rows:
            distance = abs(center_y - row["center_y"])
            if distance <= max(14.0, component["h"] * 0.35) and distance < selected_distance:
                selected_row = row
                selected_distance = distance
        if selected_row is None:
            rows.append({"center_y": center_y, "items": [component]})
        else:
            selected_row["items"].append(component)
            selected_row["center_y"] = sum(
                item["y"] + item["h"] / 2.0 for item in selected_row["items"]
            ) / len(selected_row["items"])

    rows.sort(key=lambda row: row["center_y"])
    ordered = []
    for row in rows:
        ordered.extend(sorted(row["items"], key=lambda item: item["x"]))
    return ordered


def detect_sprite_regions(magick, sheet, work_dir):
    background = run(
        [magick, str(sheet), "-format", "%[pixel:p{0,0}]", "info:"], capture=True
    ).strip()
    transparent = work_dir / f"{sheet.stem}-transparent.png"
    mask = work_dir / f"{sheet.stem}-mask.png"

    # First remove the sheet background. Building the component mask from the
    # resulting alpha channel avoids ImageMagick-version-specific colour names
    # such as gray(0), srgb(0,0,0) and srgba(0,0,0,1).
    run([
        magick, str(sheet), "-alpha", "on", "-fuzz", "2%",
        "-transparent", background, str(transparent),
    ])
    run([
        magick, str(transparent), "-alpha", "extract", "-threshold", "0",
        str(mask),
    ])

    verbose = run([
        magick, str(mask), "-define", "connected-components:verbose=true",
        "-connected-components", "8", "null:",
    ], capture=True)

    parsed = 0
    components = []
    for line in verbose.splitlines():
        match = COMPONENT_RE.match(line)
        if match is None:
            continue
        parsed += 1
        width, height, x, y, area = (int(match.group(index)) for index in range(1, 6))

        # The huge background component naturally fails these bounds. We do
        # not inspect the textual colour representation at all.
        if width >= 8 and height >= 16 and width <= 112 and height <= 112 and area >= 70:
            components.append({"x": x, "y": y, "w": width, "h": height, "area": area})

    print(
        f"{sheet.stem}: connected-components parsed={parsed}, "
        f"sprite-like={len(components)}"
    )
    if not components:
        debug_mask = sheet.parent / f"{sheet.stem}-debug-mask.png"
        shutil.copyfile(mask, debug_mask)
        raise RuntimeError(
            f"no sprite-like regions detected in {sheet}; debug mask written to {debug_mask}"
        )
    return transparent, sort_row_major(components)


def extract_frames(magick, sheet, player, frames_dir, work_dir):
    transparent, components = detect_sprite_regions(magick, sheet, work_dir)
    selected = components[:FRAME_COUNT]

    if len(selected) < FRAME_COUNT:
        print(
            f"warning: detected only {len(selected)} regions for {player}; cycling them to fill {FRAME_COUNT}",
            file=sys.stderr,
        )
        selected = [selected[index % len(selected)] for index in range(FRAME_COUNT)]
    elif len(components) > FRAME_COUNT:
        print(
            f"{player}: detected {len(components)} sprite-like regions; "
            f"using the first {FRAME_COUNT} in row-major order"
        )

    output_dir = frames_dir / player
    output_dir.mkdir(parents=True, exist_ok=True)
    for index, box in enumerate(selected):
        geometry = f"{box['w']}x{box['h']}+{box['x']}+{box['y']}"
        output = output_dir / f"{index:03d}.png"
        run([
            magick, str(transparent), "-crop", geometry, "+repage",
            "-resize", "60x60>", "-background", "black", "-gravity", "south",
            "-extent", f"{FRAME_SIZE}x{FRAME_SIZE}", str(output),
        ])
    print(f"{player}: wrote {FRAME_COUNT} normalized 64x64 frames")


def pack_frames(magick, ngdevkit, generated_dir):
    frames = []
    for player in ("p1", "p2"):
        frames.extend(
            str(generated_dir / "frames" / player / f"{index:03d}.png")
            for index in range(FRAME_COUNT)
        )

    combined = generated_dir / "fighters.png"
    indexed = generated_dir / "fighters.gif"
    run([magick, *frames, "-append", str(combined)])
    run([magick, str(combined), "+dither", "-colors", "16", "-type", "Palette", str(indexed)])

    python = os.environ.get("PYTHON", sys.executable)
    run([
        python, str(ngdevkit / "tools" / "tiletool.py"), "--sprite", "-c", str(indexed),
        "-o", str(generated_dir / "fighters.c1"), str(generated_dir / "fighters.c2"),
    ])
    run([
        python, str(ngdevkit / "tools" / "paltool.py"), str(indexed),
        "-o", str(generated_dir / "fighters.pal"),
    ])


def main():
    parser = argparse.ArgumentParser(
        description="Download KOF R-2 sheets from The Spriters Resource and build Neo Geo C-ROM assets."
    )
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    arguments = parser.parse_args()

    root = arguments.root.resolve()
    generated = root / "assets" / "generated"
    raw = generated / "raw"
    frames = generated / "frames"
    ngdevkit = (root / ".." / "external" / "ngdevkit").resolve()

    if not (ngdevkit / "tools" / "tiletool.py").exists():
        raise SystemExit("ngdevkit is missing; run ../scripts/setup-deps.sh first")

    generated.mkdir(parents=True, exist_ok=True)
    raw.mkdir(parents=True, exist_ok=True)
    frames.mkdir(parents=True, exist_ok=True)
    magick = find_magick()

    try:
        sheets = [download_sheet(magick, asset, raw) for asset in ASSETS]
        with tempfile.TemporaryDirectory(prefix="unsigned-versus-") as temporary:
            work_dir = Path(temporary)
            for asset, sheet in zip(ASSETS, sheets):
                extract_frames(magick, sheet, asset["player"], frames, work_dir)
        pack_frames(magick, ngdevkit, generated)
    except (RuntimeError, OSError, urllib.error.URLError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"asset preparation failed: {error}")

    (generated / ".stamp").write_text("generated\n", encoding="utf-8")
    print(f"generated {generated / 'fighters.c1'}, fighters.c2 and fighters.pal")


if __name__ == "__main__":
    main()
