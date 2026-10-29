#!/usr/bin/env python3
"""Scoped format/static gates; preserved upstream sources are never rewritten."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MAINTAINED = ["examples/standalone_demo.cpp", "test/test_core_boundaries.cpp", "test/install_consumer/main.cpp"]
STATIC = ["src/grid_planner.cpp"] + MAINTAINED


def run(command):
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    print(result.stdout, end="")
    print(result.stderr, end="")
    return {"command": command, "returncode": result.returncode,
            "stdout": result.stdout, "stderr": result.stderr}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--format", action="store_true")
    parser.add_argument("--static", action="store_true")
    parser.add_argument("--cppcheck", default=shutil.which("cppcheck"))
    parser.add_argument("--clang-format", default=shutil.which("clang-format"))
    parser.add_argument("--fix-format", action="store_true")
    args = parser.parse_args()
    reports = []
    if args.format or args.fix_format:
        if not args.clang_format:
            raise SystemExit("clang-format missing; use pinned requirements-dev.txt or --clang-format")
        command = [args.clang_format]
        command += ["-i"] if args.fix_format else ["--dry-run", "--Werror"]
        reports.append(run(command + MAINTAINED))
    if args.static:
        checker = args.cppcheck
        if not checker:
            raise SystemExit("cppcheck missing; this check is Not Verified")
        help_text = subprocess.check_output([checker, "--help"], text=True)
        thorough = ["--check-level=exhaustive"] if "--check-level" in help_text else ["--force"]
        reports.append(run([checker, "--enable=warning,performance,portability",
                            "--std=c++14", "--suppress=missingIncludeSystem", "--error-exitcode=1",
                            "-Iinclude"] + thorough + STATIC))
    if not reports:
        parser.error("select --format and/or --static")
    output = ROOT / "docs/validation/current"
    output.mkdir(parents=True, exist_ok=True)
    (output / "quality.json").write_text(json.dumps(reports, indent=2) + "\n", encoding="utf-8")
    raise SystemExit(any(item["returncode"] != 0 for item in reports))


if __name__ == "__main__":
    main()
