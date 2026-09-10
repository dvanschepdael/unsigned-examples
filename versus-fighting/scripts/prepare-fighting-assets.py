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

from PIL import Image, ImageDraw

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


def is_character_region(width, height, area):
    """Reject labels, separators, palette strips and tiny detached effects.

    KOF R-2 fighters are compact but still roughly character-shaped.  The old
    importer accepted regions as short as 16 px and therefore selected wide
    horizontal decorations before it ever reached the actual fighter poses.
    """
    if width < 12 or height < 24 or width > 96 or height > 96:
        return False

    aspect = width / float(height)
    if aspect < 0.28 or aspect > 2.0:
        return False

    # Connected-component area is measured on the dilated mask. Character
    # bodies have substantially more foreground than text fragments/lines.
    if area < 180:
        return False

    return True


def detect_sprite_regions(magick, sheet, work_dir):
    background = run(
        [magick, str(sheet), "-format", "%[pixel:p{0,0}]", "info:"], capture=True
    ).strip()
    transparent = work_dir / f"{sheet.stem}-transparent.png"
    mask = work_dir / f"{sheet.stem}-mask.png"

    run([
        magick, str(sheet), "-alpha", "on", "-fuzz", "2%",
        "-transparent", background, str(transparent),
    ])
    run([
        magick, str(transparent), "-alpha", "extract", "-threshold", "0",
        "-morphology", "Close", "Diamond:1",
        "-morphology", "Dilate", "Diamond:1",
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
        if is_character_region(width, height, area):
            components.append({"x": x, "y": y, "w": width, "h": height, "area": area})

    components = sort_row_major(components)
    print(f"{sheet.stem}: connected-components parsed={parsed}, character-like={len(components)}")

    if len(components) < FRAME_COUNT:
        debug_mask = sheet.parent / f"{sheet.stem}-debug-mask.png"
        shutil.copyfile(mask, debug_mask)
        raise RuntimeError(
            f"only {len(components)} character-like regions detected in {sheet}; "
            f"need {FRAME_COUNT}. Debug mask written to {debug_mask}"
        )

    return transparent, components


def save_selection_preview(sheet, selected, output):
    image = Image.open(sheet).convert("RGB")
    draw = ImageDraw.Draw(image)
    for index, box in enumerate(selected):
        x0 = box["x"]
        y0 = box["y"]
        x1 = x0 + box["w"] - 1
        y1 = y0 + box["h"] - 1
        draw.rectangle((x0, y0, x1, y1), outline=(255, 0, 255), width=1)
        draw.text((x0, max(0, y0 - 9)), str(index), fill=(255, 0, 255))
    image.save(output)


def extract_frames(magick, sheet, player, frames_dir, work_dir):
    transparent, components = detect_sprite_regions(magick, sheet, work_dir)
    selected = components[:FRAME_COUNT]

    output_dir = frames_dir / player
    output_dir.mkdir(parents=True, exist_ok=True)
    save_selection_preview(sheet, selected, sheet.parent / f"{player}-selection.png")

    for index, box in enumerate(selected):
        geometry = f"{box['w']}x{box['h']}+{box['x']}+{box['y']}"
        output = output_dir / f"{index:03d}.png"
        run([
            magick, str(transparent), "-crop", geometry, "+repage",
            "-resize", "60x60>", "-background", "none", "-gravity", "south",
            "-extent", f"{FRAME_SIZE}x{FRAME_SIZE}", str(output),
        ])

        # Never silently ship a nearly-flat frame again. Such a frame is a
        # sheet decoration, not a fighter pose.
        with Image.open(output).convert("RGBA") as frame:
            alpha_box = frame.getchannel("A").getbbox()
            if alpha_box is None or (alpha_box[3] - alpha_box[1]) < 20:
                raise RuntimeError(
                    f"invalid fighter frame {player}/{index:03d}: visible height is too small; "
                    f"inspect assets/generated/raw/{player}-selection.png"
                )

    print(f"{player}: wrote {FRAME_COUNT} normalized 64x64 fighter frames")


def build_indexed_atlas(generated_dir):
    frame_paths = []
    for player in ("p1", "p2"):
        frame_paths.extend(
            generated_dir / "frames" / player / f"{index:03d}.png"
            for index in range(FRAME_COUNT)
        )

    atlas = Image.new("RGBA", (FRAME_SIZE, FRAME_SIZE * len(frame_paths)), (0, 0, 0, 0))
    for index, frame_path in enumerate(frame_paths):
        frame = Image.open(frame_path).convert("RGBA")
        atlas.alpha_composite(frame, (0, index * FRAME_SIZE))

    atlas.save(generated_dir / "fighters.png")

    # Neo Geo sprite colour index 0 is transparent. Reserve it explicitly,
    # then place the 15-colour quantized artwork in indices 1..15.
    rgb = Image.new("RGB", atlas.size, (0, 0, 0))
    rgb.paste(atlas.convert("RGB"), mask=atlas.getchannel("A"))
    quantized = rgb.quantize(colors=15, dither=Image.Dither.NONE)
    source_palette = quantized.getpalette()[:45]

    indexed = Image.new("P", atlas.size, 0)
    palette = [0, 0, 0] + source_palette
    palette.extend([0] * (768 - len(palette)))
    indexed.putpalette(palette)

    alpha = atlas.getchannel("A")
    qdata = list(quantized.getdata())
    adata = list(alpha.getdata())
    indexed.putdata([0 if a == 0 else min(15, q + 1) for q, a in zip(qdata, adata)])
    indexed.info["transparency"] = 0
    indexed.save(generated_dir / "fighters.gif", transparency=0)


def pack_frames(ngdevkit, generated_dir):
    build_indexed_atlas(generated_dir)
    indexed = generated_dir / "fighters.gif"
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
        pack_frames(ngdevkit, generated)
    except (RuntimeError, OSError, urllib.error.URLError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"asset preparation failed: {error}")

    (generated / ".stamp").write_text("generated\n", encoding="utf-8")
    print(f"generated {generated / 'fighters.c1'}, fighters.c2 and fighters.pal")


if __name__ == "__main__":
    main()
