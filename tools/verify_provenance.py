#!/usr/bin/env python3
"""Verify unchanged upstream files, allowing intentional packaging changes."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROTECTED = ["src/grid_planner.cpp", "src/ccpp_planner_node.cpp",
             "include/ccpp_coverage_planner/grid_planner.h", "srv/PlanCoverage.srv",
             "test/test_grid_planner.cpp"]


def main():
    manifest = json.loads((ROOT / "docs/upstream_manifest.json").read_text(encoding="utf-8"))
    failures = []
    for name in PROTECTED:
        # Normalize checkout line endings; original manifest hashes canonical LF git blobs.
        data = (ROOT / name).read_bytes().replace(b"\r\n", b"\n")
        actual = hashlib.sha256(data).hexdigest()
        if actual != manifest["files"][name]:
            failures.append(name)
        print(("FAIL " if name in failures else "PASS ") + name)
    print("Upstream commit:", manifest["commit"])
    if failures:
        raise SystemExit("Protected upstream files differ: " + ", ".join(failures))
    print("All five protected upstream files match. Packaging changes are intentional; see PROVENANCE.md.")


if __name__ == "__main__":
    main()
