# Unit Test Report

**Target:** Python replay/tool contracts and preserved-core boundary probes  
**Language:** Python (core probes call actual C++ binaries)  
**Framework:** pytest 7.4.4  
**Test File:** test/test_tools.py; test/test_known_contracts.py

## Summary

| Metric | Count |
|---|---:|
| Total tests | 29 |
| Passed | 27 |
| Failed | 0 |
| Expected failures | 2 |
| Skipped | 0 |

## Passed tests

- Valid single-row replay and all six actual recorded scenarios.
- Reject malformed dimensions, layer types/values, missing/null data, invalid points/types/chunks.
- Reject repeated, skipped, nonlattice or missing coverage goals.
- Reject blocked route points and diagonal demo edges.
- Verify manifest/embedding separately through check_project and browser checks.

## Failed tests

No unexpected test failure in the final pytest run.

## Expected failures

- H02: desired rejection of occupied/outside direct-core starts. Actual upstream behavior accepts them.
- H03: desired rejection of infinite resolution. Actual Grid.valid accepts it.

Both use strict xfail. An unexpected fix produces XPASS and fails the suite for review. A missing binary produces a visible skip rather than a fabricated pass; the actual local run built the binary and has no skips.

## C++ result

14 CTest entries passed. One entry invokes 11 original GTest cases; six added boundary cases and synthetic scene/invariant checks have actual runtime assertions. Do not sum nested counts. Exact execution output is in validation/current.

## ROS result

No ROS node/service test was executed on this Windows host. Positive completion and negative service/confirmation tests are prepared for real Noetic; their status is Not Verified.

## Coverage

Coverage instrumentation was not run. No coverage percentage is claimed.

Run again with `python -m pytest -q` after building the boundary executable. Raw JUnit evidence is validation/current/python.xml.
