#!/usr/bin/env python3
"""Read-only consistency/provenance checks; writes only its explicit JSON report."""
import ast
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

import yaml
from replay_data import load_all

ROOT = Path(__file__).resolve().parents[1]
IGNORE = {".git", ".maintenance", "node_modules", "__pycache__", ".pytest_cache"}


def included(path):
    return not any(p in IGNORE or p.startswith("build") for p in path.relative_to(ROOT).parts)


def main():
    files = sorted(p for p in ROOT.rglob("*") if p.is_file() and included(p))
    result = {"status": "passed", "files_read": len(files), "python_syntax": 0,
              "xml": 0, "yaml": 0, "local_links": 0}
    for path in files:
        # Read every included file, including generated resources.
        raw = path.read_bytes()
        relative = path.relative_to(ROOT).as_posix()
        if path.suffix == ".py":
            ast.parse(raw.decode("utf-8"), filename=relative)
            result["python_syntax"] += 1
        if path.suffix in (".xml", ".launch", ".test", ".svg"):
            ET.fromstring(raw)
            result["xml"] += 1
        if path.suffix in (".yaml", ".yml", ".rviz", ".cff"):
            yaml.load(raw.decode("utf-8"), Loader=yaml.BaseLoader)
            result["yaml"] += 1
        if path.suffix == ".md":
            for target in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)', raw.decode("utf-8")):
                if re.match(r"[a-z]+://", target) or target.startswith("#"):
                    continue
                target = target.split("#", 1)[0]
                if target:
                    assert (path.parent / target).exists(), (relative, target)
                    result["local_links"] += 1
        if path.suffix in (".cpp", ".h", ".py", ".cjs") and relative != "tools/check_project.py":
            text = raw.decode("utf-8")
            assert not re.search(r"(?<![A-Za-z0-9])[A-Za-z]:[\\/]", text), "machine path in " + relative
    package = ET.parse(ROOT / "package.xml").getroot()
    assert package.findtext("name") == "ccpp_coverage_planner"
    assert package.findtext("license") == "BSD-3-Clause"
    config = yaml.safe_load((ROOT / "config/ccpp.yaml").read_text(encoding="utf-8"))
    api = (ROOT / "docs/ROS_API.md").read_text(encoding="utf-8")
    assert all(key in api for key in config), "YAML parameter absent from API docs"
    node = (ROOT / "src/ccpp_planner_node.cpp").read_text(encoding="utf-8")
    for topic in set(re.findall(r'"/ccpp/[^"]+"', node)):
        assert topic.strip('"') in api, "ROS interface absent from API docs"
    subprocess.run([sys.executable, "tools/verify_provenance.py"], cwd=ROOT, check=True)
    scenes = load_all(ROOT / "docs/data")
    assert len(scenes) == 6
    manifest = json.loads((ROOT / "docs/data/manifest.json").read_text(encoding="utf-8"))
    for name, expected in manifest["files"].items():
        assert hashlib.sha256((ROOT / "docs/data" / name).read_bytes()).hexdigest() == expected
    text = (ROOT / "docs/demo.html").read_text(encoding="utf-8")
    embedded = re.search(r'<script id="replay-data" type="application/json">(.*?)</script>', text, re.S)
    assert embedded and json.loads(embedded.group(1)) == scenes, "stale embedded replay"
    artwork = json.loads((ROOT / "assets/PROVENANCE.json").read_text(encoding="utf-8"))
    assert artwork["source_data"] == manifest["files"]
    for name, expected in artwork["files"].items():
        assert hashlib.sha256((ROOT / "assets" / name).read_bytes()).hexdigest() == expected
    assert "node_modules" not in text and "fetch(" not in text
    result["scenes"] = len(scenes)
    result["protected_files"] = 5
    output = ROOT / "docs/validation/current"
    output.mkdir(parents=True, exist_ok=True)
    (output / "project.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
