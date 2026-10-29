"""Semantic validation shared by replay generation, tests and repository checks."""
import hashlib
import json
from pathlib import Path


def integer(value, name, minimum=0):
    if type(value) is not int or value < minimum:
        raise ValueError(name + " must be an integer >= " + str(minimum))
    return value


def validate(data):
    if not isinstance(data, dict):
        raise ValueError("replay must be an object")
    required = ("width", "height", "stride", "occupancy", "skip", "steps",
                "confirmed_targets", "chunks", "scenario", "provenance")
    if any(key not in data for key in required):
        raise ValueError("missing replay fields")
    width = integer(data["width"], "width", 1)
    height = integer(data["height"], "height", 1)
    stride = integer(data["stride"], "stride", 1)
    if width * height > 1000000:
        raise ValueError("replay map is too large")
    if not isinstance(data["scenario"], str) or not data["scenario"]:
        raise ValueError("scenario name required")
    if not isinstance(data["provenance"], str) or "synthetic" not in data["provenance"].lower():
        raise ValueError("synthetic provenance required")
    occupancy, skip, steps = data["occupancy"], data["skip"], data["steps"]
    if not isinstance(occupancy, list) or not isinstance(skip, list):
        raise ValueError("map layers must be arrays")
    if len(occupancy) != width * height or len(skip) != width * height:
        raise ValueError("map dimensions do not match layers")
    if any(type(v) is not int or v not in (-1, 0, 100) for v in occupancy):
        raise ValueError("invalid occupancy value")
    if any(type(v) is not int or v not in (0, 1) for v in skip):
        raise ValueError("invalid skip value")
    if not isinstance(steps, list) or not steps:
        raise ValueError("completed replay must contain steps")
    confirmed = set()
    last = None
    last_chunk = 0
    per_chunk = {}
    for step in steps:
        if not isinstance(step, list) or len(step) != 4:
            raise ValueError("step must be [x,y,type,chunk]")
        x, y, kind, chunk = step
        for value, label in [(x, "x"), (y, "y"), (kind, "type"), (chunk, "chunk")]:
            integer(value, label)
        if x >= width or y >= height or kind not in (0, 1):
            raise ValueError("step outside map or invalid type")
        if chunk < 1 or chunk not in (last_chunk, last_chunk + 1):
            raise ValueError("nonsequential chunks")
        last_chunk = chunk
        index = y * width + x
        if occupancy[index] != 0:
            raise ValueError("path enters obstacle or unknown cell")
        if last is not None:
            ax, ay = last
            if x != ax and y != ay:
                raise ValueError("demo edge is not axis aligned")
            dx, dy = (x > ax) - (x < ax), (y > ay) - (y < ay)
            for delta in range(abs(x - ax) + abs(y - ay) + 1):
                if occupancy[(ay + dy * delta) * width + ax + dx * delta] != 0:
                    raise ValueError("path segment crosses blocked cell")
        if kind == 1:
            if skip[index] or index in confirmed or x % stride or y % stride:
                raise ValueError("skipped, repeated or nonlattice coverage target")
            confirmed.add(index)
            per_chunk[chunk] = per_chunk.get(chunk, 0) + 1
        last = (x, y)
    # Independent graph traversal validates completeness of these lattice demos.
    sx, sy = steps[0][:2]
    pending = [(sx, sy)]
    reachable = {(sx, sy)}
    for x, y in pending:
        for dx, dy in ((stride, 0), (-stride, 0), (0, stride), (0, -stride)):
            nx, ny = x + dx, y + dy
            if not (0 <= nx < width and 0 <= ny < height) or (nx, ny) in reachable:
                continue
            step_x, step_y = (dx > 0) - (dx < 0), (dy > 0) - (dy < 0)
            if all(occupancy[(y + step_y * k) * width + x + step_x * k] == 0
                   for k in range(stride + 1)):
                reachable.add((nx, ny))
                pending.append((nx, ny))
    expected = {y * width + x for x, y in reachable if not skip[y * width + x]}
    if confirmed != expected:
        raise ValueError("reachable coverage targets missing or unexpected")
    if integer(data["confirmed_targets"], "confirmed_targets") != len(confirmed):
        raise ValueError("confirmed target total mismatch")
    if integer(data["chunks"], "chunks") != last_chunk:
        raise ValueError("chunk total mismatch")
    if any(count > 8 for count in per_chunk.values()):
        raise ValueError("synthetic demo chunk budget exceeded")
    return data


def load(path):
    return validate(json.loads(Path(path).read_text(encoding="utf-8")))


def load_all(directory):
    return [load(path) for path in sorted(Path(directory).glob("*.json"))
            if path.name != "manifest.json"]


def write_manifest(directory):
    directory = Path(directory)
    paths = sorted(p for p in directory.glob("*.json") if p.name != "manifest.json")
    for path in paths:
        load(path)
    result = {"generator": "examples/standalone_demo.cpp calling unchanged upstream GridPlanner",
              "upstream_commit": "8bc54d2bdb257b3f2f357b1709350e28d25f35cc",
              "provenance": "synthetic maps; precomputed C++ output; ideal simulated arrivals",
              "files": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    (directory / "manifest.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
