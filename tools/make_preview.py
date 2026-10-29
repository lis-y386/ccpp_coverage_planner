#!/usr/bin/env python3
"""Generate an offline/Pages-compatible replay; no JavaScript planner or fetch."""
import argparse
import json
from pathlib import Path

from replay_data import load, load_all, write_manifest

ROOT = Path(__file__).resolve().parents[1]


def render(scenes):
    if not scenes:
        raise ValueError("no replay scenes")
    template = (ROOT / "tools/demo_template.html").read_text(encoding="utf-8")
    # Escape HTML-significant text inside a script element, including </script>.
    embedded = json.dumps(scenes, ensure_ascii=True).replace("<", "\\u003c")
    return template.replace("__SCENES__", embedded)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--input", type=Path, help="Single C++ replay JSON (compatibility mode)")
    group.add_argument("--data-dir", type=Path, help="Directory containing recorded scene JSON")
    parser.add_argument("--output", type=Path, default=ROOT / "docs/demo.html")
    args = parser.parse_args()
    if args.input:
        scenes = [load(args.input)]
    else:
        directory = args.data_dir or ROOT / "docs/data"
        write_manifest(directory)
        scenes = load_all(directory)
    args.output.write_text(render(scenes), encoding="utf-8")
    print("Generated precomputed replay:", args.output, "scenes:", len(scenes))


if __name__ == "__main__":
    main()
