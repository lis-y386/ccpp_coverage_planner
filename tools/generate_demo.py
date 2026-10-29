#!/usr/bin/env python3
"""Regenerate all public replay records using the built C++ executable."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess

from replay_data import load, write_manifest

ROOT = Path(__file__).resolve().parents[1]
SCENES = ("rectangle", "obstacles", "skip", "disconnected", "stride", "maze")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    args = parser.parse_args()
    binary = args.binary.resolve()
    if not binary.is_file():
        raise SystemExit("Build ccpp_demo first: " + str(binary))
    directory = ROOT / "docs/data"
    directory.mkdir(exist_ok=True)
    records = []
    for scene in SCENES:
        command = [str(binary), "--scenario", scene, "--output", str(directory / scene)]
        result = subprocess.run(command, check=True, capture_output=True, text=True, timeout=30)
        load(directory / (scene + ".json"))
        records.append({"command": command, "output": result.stdout.strip(), "returncode": 0})
        print(result.stdout.strip())
    write_manifest(directory)
    # Keep the original single-scene paths usable.
    for suffix in (".json", ".svg"):
        shutil.copyfile(directory / ("obstacles" + suffix), ROOT / "docs" / ("demo_obstacles" + suffix))
    target = ROOT / "docs/validation/current"
    target.mkdir(parents=True, exist_ok=True)
    (target / "demo_generation.json").write_text(json.dumps(records, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
