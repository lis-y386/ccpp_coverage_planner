"""Tool/replay contracts using actual recorded maps and isolated in-memory mutations."""
from pathlib import Path
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from replay_data import load_all, validate


def sample():
    return {"scenario": "unit", "provenance": "synthetic test map", "width": 2, "height": 1,
            "stride": 1, "occupancy": [0, 0], "skip": [0, 0], "steps": [[0, 0, 1, 1], [1, 0, 1, 1]],
            "confirmed_targets": 2, "chunks": 1}


def test_valid_single_row():
    assert validate(sample())["confirmed_targets"] == 2


def test_all_recorded_scenes():
    scenes = load_all(ROOT / "docs/data")
    assert len(scenes) == 6
    assert {s["scenario"] for s in scenes} == {"rectangle", "obstacles", "skip", "disconnected", "stride", "maze"}


@pytest.mark.parametrize("field,value", [
    ("width", 0), ("height", -1), ("stride", True), ("occupancy", [0]),
    ("occupancy", [0, 13]), ("skip", [0, 3]), ("steps", []),
    ("steps", [[0, 0, 1, 1], [0, 0, 1, 1]]),
    ("steps", [[0, 0, 1, 1], [2, 0, 1, 1]]),
    ("steps", [[0, 0, 1, 1], [1, 0, 1, 3]]),
    ("steps", [[0, 0, 1, 1], [1.0, 0, 1, 1]]),
    ("confirmed_targets", 1), ("chunks", 2), ("provenance", "real flight"),
    ("scenario", None), ("skip", None), ("steps", None),
])
def test_reject_malformed_replay(field, value):
    data = sample()
    data[field] = value
    with pytest.raises(ValueError):
        validate(data)


def test_reject_blocked_route():
    data = sample()
    data["occupancy"][1] = 100
    with pytest.raises(ValueError, match="obstacle"):
        validate(data)


def test_reject_skipped_target():
    data = sample()
    data["skip"][1] = 1
    with pytest.raises(ValueError, match="skipped"):
        validate(data)


def test_reject_missing_targets():
    data = sample()
    data["steps"].pop()
    data["confirmed_targets"] = 1
    with pytest.raises(ValueError, match="missing"):
        validate(data)


@pytest.mark.parametrize("data", [None, [], {}, {"width": 2}])
def test_reject_missing_or_null(data):
    with pytest.raises(ValueError):
        validate(data)


def test_reject_diagonal():
    data = sample()
    data.update(width=2, height=2, occupancy=[0]*4, skip=[0]*4,
                steps=[[0, 0, 1, 1], [1, 1, 1, 1]])
    with pytest.raises(ValueError, match="axis aligned"):
        validate(data)
