#!/usr/bin/env python3
import argparse
import html as html_module
import os
import re
import subprocess
import sys
import tempfile
import urllib.error
import urllib.parse
import urllib.request
from collections import Counter
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
MAX_SOURCE_FRAME_SIZE = 160
USER_AGENT = "unsigned-examples educational asset builder/1.0 (+https://github.com/dvanschepdael/unsigned-examples)"


def run(command):
    subprocess.run(command, check=True, text=True)


def pixel_data(image):
    getter = getattr(image, "get_flattened_data", None)
    if getter is not None:
        return list(getter())
    return list(image.getdata())


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


def image_size(path):
    with Image.open(path) as image:
        return image.size


def download_sheet(asset, raw_dir):
    output = raw_dir / f"{asset['player']}.png"
    if output.exists() and image_size(output) == asset["size"]:
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
                size = image_size(temporary_image)
            except OSError:
                continue

            if size == asset["size"]:
                output.write_bytes(data)
                print(f"downloaded {asset['name']} sheet: {candidate}")
                return output

    raise RuntimeError(
        f"PNG candidates for {asset['name']} did not match the expected "
        f"{asset['size'][0]}x{asset['size'][1]} sheet; the source page may have changed"
    )


def longest_horizontal_run(image, color):
    pixels = image.load()
    width, height = image.size
    longest = 0
    for y in range(height):
        current = 0
        for x in range(width):
            pixel = pixels[x, y]
            if pixel[3] != 0 and pixel[:3] == color:
                current += 1
                longest = max(longest, current)
            else:
                current = 0
    return longest


def longest_vertical_run(image, color):
    pixels = image.load()
    width, height = image.size
    longest = 0
    for x in range(width):
        current = 0
        for y in range(height):
            pixel = pixels[x, y]
            if pixel[3] != 0 and pixel[:3] == color:
                current += 1
                longest = max(longest, current)
            else:
                current = 0
    return longest


def border_color_counts(image):
    width, height = image.size
    band = max(2, min(12, min(width, height) // 32))
    counts = Counter()
    total = 0
    pixels = image.load()

    for y in range(height):
        for x in range(width):
            if x >= band and x < width - band and y >= band and y < height - band:
                continue
            red, green, blue, alpha = pixels[x, y]
            if alpha == 0:
                continue
            counts[(red, green, blue)] += 1
            total += 1

    return counts, total


def detect_background_colors(image):
    """Detect flat sheet backgrounds without relying on one specific layout.

    The Spriters Resource sheets are not normalized: some use solid fields,
    some use several flat colours, and dense sheets may contain only narrow
    gaps between poses. Background colours are inferred from opaque corners,
    the outer border, dominant global colours and long straight runs.
    """
    rgba = image.convert("RGBA")
    width, height = rgba.size
    pixels = pixel_data(rgba)
    opaque = [pixel for pixel in pixels if pixel[3] != 0]
    if not opaque:
        return set()

    counts = Counter(pixel[:3] for pixel in opaque)
    border_counts, border_total = border_color_counts(rgba)
    opaque_total = len(opaque)
    backgrounds = set()

    for x, y in ((0, 0), (width - 1, 0), (0, height - 1), (width - 1, height - 1)):
        pixel = rgba.getpixel((x, y))
        if pixel[3] != 0:
            backgrounds.add(pixel[:3])

    # Catch checkerboard/striped backgrounds whose individual colour runs are
    # too short for the old long-run heuristic. A true sheet background tends
    # to dominate the outer border as well as a meaningful part of the image.
    if border_total > 0:
        for color, border_count in border_counts.most_common(16):
            global_count = counts[color]
            border_share = border_count / float(border_total)
            global_share = global_count / float(opaque_total)
            if border_share >= 0.03 and global_share >= 0.005:
                backgrounds.add(color)

    # Also catch a dominant field that may not reach every edge because the
    # author placed a frame or title around the sheet.
    for color, count in counts.most_common(16):
        global_share = count / float(opaque_total)
        if global_share >= 0.12:
            backgrounds.add(color)

    minimum_count = max(128, (width * height) // 1000)
    long_run = max(48, min(width, height) // 7)
    for color, count in counts.most_common(32):
        if count < minimum_count:
            break
        if color in backgrounds:
            continue
        if longest_horizontal_run(rgba, color) >= long_run or longest_vertical_run(rgba, color) >= long_run:
            backgrounds.add(color)

    return backgrounds


def build_foreground_images(sheet, work_dir):
    source = Image.open(sheet).convert("RGBA")
    background_colors = detect_background_colors(source)

    cleaned = source.copy()
    cleaned_pixels = []
    for red, green, blue, alpha in pixel_data(source):
        if alpha == 0 or (red, green, blue) in background_colors:
            cleaned_pixels.append((red, green, blue, 0))
        else:
            cleaned_pixels.append((red, green, blue, 255))
    cleaned.putdata(cleaned_pixels)

    # Do not dilate the mask. Dense sheets can place two poses only one or two
    # pixels apart; the previous MaxFilter(5) merged the complete Iori sheet
    # into a single 796x583 component.
    mask = cleaned.getchannel("A").point(lambda value: 255 if value else 0)

    transparent_path = work_dir / f"{sheet.stem}-transparent.png"
    mask_path = sheet.parent / f"{sheet.stem}-debug-mask.png"
    cleaned.save(transparent_path)
    mask.save(mask_path)

    colors = ", ".join(f"#{r:02x}{g:02x}{b:02x}" for r, g, b in sorted(background_colors))
    print(
        f"{sheet.stem}: removed {len(background_colors)} sheet background colours"
        f"{(' [' + colors + ']') if colors else ''}; debug mask: {mask_path}"
    )
    return cleaned, mask


def connected_components(mask):
    image = mask.convert("L")
    width, height = image.size
    data = image.tobytes()
    visited = bytearray(width * height)
    components = []

    for start in range(width * height):
        if visited[start] or data[start] == 0:
            continue

        stack = [start]
        visited[start] = 1
        min_x = max_x = start % width
        min_y = max_y = start // width
        area = 0

        while stack:
            index = stack.pop()
            x = index % width
            y = index // width
            area += 1
            min_x = min(min_x, x)
            max_x = max(max_x, x)
            min_y = min(min_y, y)
            max_y = max(max_y, y)

            for ny in range(max(0, y - 1), min(height - 1, y + 1) + 1):
                row = ny * width
                for nx in range(max(0, x - 1), min(width - 1, x + 1) + 1):
                    neighbor = row + nx
                    if not visited[neighbor] and data[neighbor] != 0:
                        visited[neighbor] = 1
                        stack.append(neighbor)

        components.append({
            "x": min_x,
            "y": min_y,
            "w": max_x - min_x + 1,
            "h": max_y - min_y + 1,
            "area": area,
        })

    return components


def component_is_plausible(component):
    width = component["w"]
    height = component["h"]
    area = component["area"]

    if width < 8 or height < 18 or area < 60:
        return False
    if width > MAX_SOURCE_FRAME_SIZE or height > MAX_SOURCE_FRAME_SIZE:
        return False
    if width >= height * 4:
        return False
    if height >= width * 8:
        return False
    return True


def component_score(component):
    width = component["w"]
    height = component["h"]
    area = component["area"]
    aspect = width / float(max(1, height))
    shape_penalty = abs(aspect - 0.8) * 80.0
    return float(area) + float(height * 16) - shape_penalty


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


def detect_sprite_regions(sheet, work_dir):
    cleaned, mask = build_foreground_images(sheet, work_dir)
    parsed = connected_components(mask)
    candidates = [component for component in parsed if component_is_plausible(component)]

    print(f"{sheet.stem}: foreground components={len(parsed)}, plausible poses={len(candidates)}")

    if len(candidates) < FRAME_COUNT:
        ranked = sorted(parsed, key=component_score, reverse=True)
        print(f"{sheet.stem}: top foreground components:", file=sys.stderr)
        for component in ranked[:min(40, len(ranked))]:
            print(
                f"  {component['w']}x{component['h']}+{component['x']}+{component['y']} "
                f"area={component['area']}",
                file=sys.stderr,
            )
        raise RuntimeError(
            f"only {len(candidates)} plausible fighter poses detected in {sheet}; need {FRAME_COUNT}. "
            f"Inspect {sheet.parent / (sheet.stem + '-debug-mask.png')}"
        )

    selected = sorted(candidates, key=component_score, reverse=True)[:FRAME_COUNT]
    return cleaned, sort_row_major(selected)


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


def normalize_frame(cleaned, box):
    crop = cleaned.crop((box["x"], box["y"], box["x"] + box["w"], box["y"] + box["h"]))
    alpha_box = crop.getchannel("A").getbbox()
    if alpha_box is None:
        raise RuntimeError("selected pose contains no visible pixels")
    crop = crop.crop(alpha_box)

    if crop.width > 60 or crop.height > 60:
        scale = min(60.0 / crop.width, 60.0 / crop.height)
        size = (max(1, int(crop.width * scale)), max(1, int(crop.height * scale)))
        crop = crop.resize(size, Image.Resampling.NEAREST)

    frame = Image.new("RGBA", (FRAME_SIZE, FRAME_SIZE), (0, 0, 0, 0))
    x = (FRAME_SIZE - crop.width) // 2
    y = FRAME_SIZE - crop.height
    frame.alpha_composite(crop, (x, y))
    return frame


def extract_frames(sheet, player, frames_dir, work_dir):
    cleaned, selected = detect_sprite_regions(sheet, work_dir)
    output_dir = frames_dir / player
    output_dir.mkdir(parents=True, exist_ok=True)
    save_selection_preview(sheet, selected, sheet.parent / f"{player}-selection.png")

    for index, box in enumerate(selected):
        frame = normalize_frame(cleaned, box)
        alpha_box = frame.getchannel("A").getbbox()
        if alpha_box is None or (alpha_box[3] - alpha_box[1]) < 18:
            raise RuntimeError(
                f"invalid fighter frame {player}/{index:03d}; "
                f"inspect assets/generated/raw/{player}-selection.png"
            )
        frame.save(output_dir / f"{index:03d}.png")

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
        with Image.open(frame_path) as source:
            frame = source.convert("RGBA")
        atlas.alpha_composite(frame, (0, index * FRAME_SIZE))

    atlas.save(generated_dir / "fighters.png")

    rgb = Image.new("RGB", atlas.size, (0, 0, 0))
    rgb.paste(atlas.convert("RGB"), mask=atlas.getchannel("A"))
    quantized = rgb.quantize(colors=15, dither=Image.Dither.NONE)
    source_palette = quantized.getpalette()[:45]

    indexed = Image.new("P", atlas.size, 0)
    palette = [0, 0, 0] + source_palette
    palette.extend([0] * (768 - len(palette)))
    indexed.putpalette(palette)

    alpha = atlas.getchannel("A")
    qdata = pixel_data(quantized)
    adata = pixel_data(alpha)
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

    try:
        sheets = [download_sheet(asset, raw) for asset in ASSETS]
        with tempfile.TemporaryDirectory(prefix="unsigned-versus-") as temporary:
            work_dir = Path(temporary)
            for asset, sheet in zip(ASSETS, sheets):
                extract_frames(sheet, asset["player"], frames, work_dir)
        pack_frames(ngdevkit, generated)
    except (RuntimeError, OSError, urllib.error.URLError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"asset preparation failed: {error}")

    (generated / ".stamp").write_text("generated\n", encoding="utf-8")
    print(f"generated {generated / 'fighters.c1'}, fighters.c2 and fighters.pal")


if __name__ == "__main__":
    main()
