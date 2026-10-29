"""Desired core contracts: strict expected failures for preserved upstream defects."""
import os
from pathlib import Path
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]
SUFFIX = ".exe" if os.name == "nt" else ""
BINARY = Path(os.environ.get("CCPP_BOUNDARY_BINARY", ROOT / "build" / ("test_core_boundaries" + SUFFIX)))


@pytest.mark.parametrize("probe,reason", [
    pytest.param("--probe-invalid-start", "H02", marks=pytest.mark.xfail(strict=True, reason="H02: invalid core start accepted")),
    pytest.param("--probe-nonfinite-grid", "H03", marks=pytest.mark.xfail(strict=True, reason="H03: nonfinite resolution accepted")),
])
def test_desired_rejection_contract(probe, reason):
    if not BINARY.is_file():
        pytest.skip("Build test_core_boundaries first or set CCPP_BOUNDARY_BINARY")
    result = subprocess.run([str(BINARY), probe], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
