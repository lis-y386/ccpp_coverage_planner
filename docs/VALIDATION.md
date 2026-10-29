# Validation record

## Current update — 2026-10-29

Private Linux CI now passes all three jobs, including headless ROS Noetic devel/install-space tests. See [CLOUD_VALIDATION.md](CLOUD_VALIDATION.md). H02/H03 and real hardware/GUI verification remain open; no public release was made.

The assessment below is the dated initial record from 2026-10-26; its remote-CI/ROS Not Verified statements are superseded only within the scope of the cloud report.

## Initial local assessment (historical)

Maintenance began 2026-10-25; final review 2026-10-26 (Asia/Shanghai). This is an unpublished local snapshot. Results are evidence for the named environment only.

## Executed results

| Check | Result | Evidence |
|---|---|---|
| Native Release / C++14 / MinGW GCC 13.2 | Passed | Actual configure/build commands in validation/current |
| CTest | 14/14 entries passed | Includes the original GTest executable as one entry |
| Original upstream GTest | 11/11 cases passed | Built GoogleTest v1.14.0 from source with matching MinGW ABI |
| New boundary assertions | Passed | Empty/mismatched grid, nonpositive resolution, no FREE cells, single target, confirmations, thin obstacles, UNKNOWN barriers |
| Python tool/replay tests | 27 passed; 2 strict expected failures | python.xml; pytest output |
| Core illegal-start rejection | **Known defect / xfail** | H02 probe accepts occupied and adjacent outside starts |
| Infinite-resolution rejection | **Known defect / xfail** | H03 probe accepts infinite resolution |
| Standalone installation / exported target consumer | Passed | install_quality_commands.json and actual consumer exit 0 |
| Replay semantics + source/data hashes | Passed | check_project.py and provenance output |
| Browser | Passed | Six exact embedded datasets, final counts, play/pause, speed, seek, reset, route toggle, responsive layout, no HTTP requests/page errors |
| Scoped clang-format 18.1.8 | Passed | Maintained example/new tests only; original source preserved |
| Cppcheck 2.14.0 | Passed after tool setup | Core/example/new native tests, warning/performance/portability, exhaustive branches |
| Python/XML/YAML/CFF syntax and local document links | See current project/schema reports | Actual consistency checks; CFF schema evidence is separate |
| Original protected files | Five match upstream hashes | verify_provenance.py |

CTest and GTest counts overlap. The Python xfails are reproduced bugs, not successful input rejection. No test-coverage percentage, performance benchmark or formal safety proof is claimed.

## Reproducible commands

Basic native/GTest sequence used a local GoogleTest source checkout (tag v1.14.0, commit f8d7d77c06936315286eb55f8de22cd23c188571):

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCCPP_STANDALONE=ON -DCCPP_BUILD_UPSTREAM_TESTS=ON -DCCPP_GTEST_SOURCE_DIR="$PWD/.maintenance/vendor/googletest" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS=-static
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
.\build\test_grid_planner.exe
python -m pytest -q --junitxml=docs/validation/current/python.xml
python tools/verify_provenance.py
python tools/check_project.py
python tools/generate_demo.py --binary build/ccpp_demo.exe
python tools/make_preview.py
python tools/render_assets.py
```

Actual machine invocations additionally supplied the installed MinGW compiler and gmake paths. Raw command arrays and results are retained in validation/current/final_commands.json; commands in this document are portable equivalents.

```text
python tools/check_quality.py --format --static --clang-format <local-clang-format> --cppcheck <local-cppcheck>
cmake --install build --prefix <project>/.maintenance/install
cmake -S test/install_consumer -B build-consumer -DCMAKE_PREFIX_PATH=<project>/.maintenance/install
cmake --build build-consumer --parallel 2
build-consumer/consumer.exe
node tools/test_browser.cjs
```

Angle-bracket values above describe selected local tools, not literal executable commands. Fully resolved actual command arrays and environment overrides are in the machine report. No command modified the flight project or published a repository.

## Synthetic native scene results

| Scene | Confirmed reachable goals | Chunks | Replay points |
|---|---:|---:|---:|
| rectangle | 320 | 40 | 359 |
| obstacles | 292 | 37 | 368 |
| skip | 304 | 38 | 357 |
| disconnected | 144 | 18 | 161 |
| stride | 72 | 9 | 92 |
| maze | 281 | 36 | 355 |

Source JSON and SHA256: data/manifest.json. These are synthetic target counts, not physical area coverage, planning speed or flight results. GIF frames are sampled from these actual records; animation duration has no performance meaning.

## Failed attempts retained

- Baseline had an incompatible prebuilt GTest. This pass built matching GoogleTest source and executed the original 11 tests.
- System Cppcheck could not locate std.cfg. Supplying --library alone did not repair automatic std loading.
- A local relocated copy initially failed to load a matching runtime. Its recorded Windows status is 3221225785.
- With adjacent cfg files and matching runtime search path, analysis ran; default branch limiting produced an informational nonzero exit. The final run uses supported exhaustive analysis. The wrapper falls back to --force on older Cppcheck versions.

These are tool/environment setup failures, not hidden passing tests. First-attempt and install/quality logs remain in validation/current.

## Not executed / environment unavailable

The current host has no usable ROS/WSL/Docker environment. The following remain **Not Verified**, not skipped-as-passed:

- Ubuntu 20.04 / Noetic catkin build and dependency resolution.
- Positive and negative ROS service/node tests.
- Catkin installed-space rostest, TF fallback behavior and RViz visual runtime.
- Docker image build, APT/rosdep resolution and image digest.
- Remote GitHub Actions jobs, Pages hosting and public artifact downloads.
- Linux/macOS/MSVC/ARM runtime, sanitizers/fuzzing and physical robot validation.

CI and container configurations are prepared with real assertions, timeouts and failure propagation, but their existence is not execution evidence. See RELEASE_READINESS.md for gates.

Historical baseline outputs (the old six-check log and initial local_checks.json) remain for traceability. Current evidence is under validation/current; baseline results must not be substituted for current results.
