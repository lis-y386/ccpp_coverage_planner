#!/usr/bin/env python3
"""Render original project artwork from validated C++ records; no invented routes."""
import hashlib
import html
import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from replay_data import load_all

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
INK, TEAL, BLOCK, AMBER = "#203a47", "#177f7b", "#344c58", "#f1d49b"


def svg_map(scene, ox, oy, cell):
    parts = []
    def center(x, y):
        return ox + (x + .5) * cell, oy + (scene["height"] - y - .5) * cell
    for y in range(scene["height"]):
        for x in range(scene["width"]):
            i = y * scene["width"] + x
            color = BLOCK if scene["occupancy"][i] == 100 else AMBER if scene["skip"][i] else "#f7faf7"
            px, py = center(x, y)
            parts.append(f'<rect x="{px-cell/2}" y="{py-cell/2}" width="{cell}" height="{cell}" fill="{color}" stroke="#d9e5dc" stroke-width=".4"/>')
    points = " ".join(f"{center(s[0],s[1])[0]},{center(s[0],s[1])[1]}" for s in scene["steps"])
    parts.append(f'<polyline points="{points}" fill="none" stroke="{TEAL}" stroke-width="1.3" opacity=".7"/>')
    return "".join(parts)


def save_svg(name, width, height, body):
    text = f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}"><g font-family="Arial, sans-serif">{body}</g></svg>\n'
    (ASSETS / name).write_bytes(text.encode("utf-8"))


def font(size):
    for name in ("DejaVuSans.ttf", "segoeui.ttf", "Arial.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def main():
    ASSETS.mkdir(exist_ok=True)
    scenes = load_all(ROOT / "docs/data")
    scene = next(s for s in scenes if s["scenario"] == "obstacles")
    body = '<rect width="1280" height="455" rx="16" fill="#173943"/>'
    body += '<text x="44" y="62" fill="#91d4c5" font-size="14" letter-spacing="3">CCPP / COVERAGE PLANNING</text>'
    body += '<text x="44" y="139" fill="#ffffff" font-size="45" font-weight="700">From free cells</text><text x="44" y="193" fill="#ffffff" font-size="45" font-weight="700">to confirmed coverage.</text>'
    body += '<text x="46" y="245" fill="#cadbd9" font-size="19">Boustrophedon + A* + explicit arrivals</text>'
    body += '<text x="46" y="300" fill="#93d4c4" font-size="15">C++14 CORE  /  ROS1 ADAPTER  /  OFFLINE REPLAY</text>'
    body += '<text x="46" y="389" fill="#cadbd9" font-size="13">Actual C++ output on a synthetic map. Ideal arrivals only.</text>'
    body += '<rect x="758" y="35" width="470" height="385" rx="12" fill="#f4f7f3"/>'
    body += svg_map(scene, 786, 65, 20)
    save_svg("banner.svg", 1280, 455, body)

    parts = ['<rect width="1080" height="720" fill="#f4f6f2"/>',
             f'<text x="28" y="36" fill="{INK}" font-size="23" font-weight="700">Six synthetic maps. One unchanged C++ core.</text>']
    for index, item in enumerate(scenes):
        col, row = index % 3, index // 3
        ox, oy = 28 + col * 355, 87 + row * 310
        parts.append(f'<text x="{ox}" y="{oy-14}" fill="{INK}" font-size="16">{html.escape(item["scenario"])} / {item["confirmed_targets"]} targets</text>')
        parts.append(svg_map(item, ox, oy, 14))
        parts.append(f'<text x="{ox}" y="{oy+248}" fill="#677b81" font-size="11">{item["chunks"]} chunks; ideal simulated confirmations</text>')
    parts.append('<text x="28" y="704" fill="#677b81" font-size="12">Disconnected free regions may remain unconfirmed. No flight or timing claim.</text>')
    save_svg("scenes.svg", 1080, 720, "".join(parts))

    body = '<rect width="1160" height="350" fill="#f5f7f3"/>'
    boxes = [(25,85,220,"Map + odometry","Obstacle cloud / TF"),
             (300,85,260,"ROS adapter","Occupancy / confirmed state"),
             (615,85,220,"C++ core","Lattice sweep + A*"),
             (890,85,245,"External executor","Actual motion / arrival")]
    for x,y,w,title,sub in boxes:
        body += f'<rect x="{x}" y="{y}" width="{w}" height="100" rx="10" fill="#e3eee7" stroke="#94b6aa"/>'
        body += f'<text x="{x+15}" y="{y+35}" fill="{INK}" font-size="18">{title}</text><text x="{x+15}" y="{y+65}" fill="#607981" font-size="13">{sub}</text>'
    body += '<g stroke="#177f7b" stroke-width="2" fill="none"><path d="M245 135H300 M560 135H615 M835 135H890"/><path d="M1010 185V260H430V185"/></g>'
    body += '<text x="560" y="248" fill="#177f7b" font-size="15">Actual reached_point acknowledgment</text><text x="25" y="36" fill="#203a47" font-size="22">Planning is separate from physical execution.</text><text x="25" y="326" fill="#607981" font-size="13">Standalone mode calls the same C++ core directly; no ROS or flight stack is required.</text>'
    save_svg("architecture.svg", 1160, 350, body)

    badges = {"license": ("license", "BSD-3-Clause", "#39765a"), "cpp": ("core", "C++14", "#397083"),
              "ros": ("ROS", "Noetic / legacy", "#6a6a52"), "ci": ("CI", "see validation", "#806840")}
    (ASSETS / "badges").mkdir(exist_ok=True)
    for name, (label, value, color) in badges.items():
        width = 85 + max(85, len(value) * 7 + 18)
        badge = f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="24" role="img" aria-label="{html.escape(label+": "+value)}"><rect width="{width}" height="24" rx="4" fill="{color}"/><rect width="85" height="24" rx="4" fill="#344c58"/><g fill="white" font-family="Arial,sans-serif" font-size="11" text-anchor="middle"><text x="42" y="16">{label}</text><text x="{85+(width-85)/2}" y="16">{html.escape(value)}</text></g></svg>'
        (ASSETS / "badges" / (name + ".svg")).write_text(badge, encoding="utf-8")

    frames = []
    total = len(scene["steps"])
    for frame in range(55):
        end = round(total * frame / 54)
        img = Image.new("RGB", (800, 515), "#f3f5f1")
        draw = ImageDraw.Draw(img)
        draw.text((28,16), "CCPP / actual C++ output, synthetic map", fill=INK, font=font(20))
        draw.text((28,45), "Precomputed replay | ideal arrivals | illustrative timing", fill="#62797c", font=font(12))
        unit, ox, oy = 23, 30, 79
        def point(step):
            return ox + (step[0]+.5)*unit, oy+(scene["height"]-step[1]-.5)*unit
        visited = {s[1]*scene["width"]+s[0] for s in scene["steps"][:end] if s[2] == 1}
        for y in range(scene["height"]):
            for x in range(scene["width"]):
                i = y*scene["width"]+x
                color = BLOCK if scene["occupancy"][i] == 100 else "#bde2d6" if i in visited else "#fafcf8"
                px, py = point((x,y))
                draw.rectangle((px-unit/2,py-unit/2,px+unit/2,py+unit/2),fill=color,outline="#d8e3da")
        points = [point(s) for s in scene["steps"][:end]]
        if len(points)>1:
            draw.line(points,fill=TEAL,width=2)
        if points:
            x,y=points[-1]
            draw.ellipse((x-5,y-5,x+5,y+5),fill="#ce7548")
        draw.text((530,110),"Confirmed targets",fill="#62797c",font=font(14))
        draw.text((530,138),f'{len(visited)} / {scene["confirmed_targets"]}',fill=TEAL,font=font(28))
        draw.text((530,205),"Chunk",fill="#62797c",font=font(14))
        chunk=scene["steps"][end-1][3] if end else 0
        draw.text((530,233),f'{chunk} / {scene["chunks"]}',fill=TEAL,font=font(28))
        draw.text((530,320),"No flight validation",fill="#62797c",font=font(14))
        draw.text((28,480),"Targets are lattice goals; not a continuous sensor-footprint coverage guarantee.",fill="#62797c",font=font(12))
        frames.append(img.convert("P", palette=Image.Palette.ADAPTIVE))
    durations = [90]*len(frames)
    durations[-1] = 1200
    frames[0].save(ASSETS/"demo.gif",save_all=True,append_images=frames[1:],duration=durations,loop=0,disposal=2,optimize=False)
    sources=json.loads((ROOT/"docs/data/manifest.json").read_text(encoding="utf-8"))
    manifest={"creator":"tools/render_assets.py", "license":"BSD-3-Clause",
              "source_data":sources["files"],"provenance":"Original artwork generated from actual C++ output on synthetic maps; system fonts are not bundled.",
              "files":{p.relative_to(ASSETS).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
                       for p in ASSETS.rglob("*") if p.is_file() and p.suffix in (".svg",".gif")}}
    (ASSETS/"PROVENANCE.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    print("Generated banner, architecture, six-scene comparison, badges and",len(frames),"GIF frames")


if __name__ == "__main__":
    main()
