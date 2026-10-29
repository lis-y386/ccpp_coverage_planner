# Cloud validation — 2026-10-29

**Result: all three private GitHub Actions jobs passed.** This supersedes the earlier “remote CI / headless ROS not verified” status for the exact tested snapshot. It does not validate RViz GUI, real sensor maps, a simulator's physical dynamics, or an aircraft.

- Repository (validation ran while private): [lis-y386/ccpp_coverage_planner](https://github.com/lis-y386/ccpp_coverage_planner)
- Successful run: [40881178486](https://github.com/lis-y386/ccpp_coverage_planner/actions/runs/40881178486)
- Tested commit: `f28a7c194e4ebcf6f02afbcc9b55da2a5f6987a3`
- Upstream protected sources still match `8bc54d2bdb257b3f2f357b1709350e28d25f35cc`.
- These runs were performed while the repository was private. Subsequent alpha publication is described in RELEASE_READINESS.md. No registry image or Pages deployment is included. The original drone repository and flight workspace were not modified.

## Actual job results

| Job | Environment / executed checks | Result |
|---|---|---|
| native | Ubuntu 22.04; C++ build, CTest, original GTest, Python contracts, provenance, project consistency, format/static analysis, installed-library consumer | Passed |
| browser | Linux / Node 22 / Playwright; all six exact C++ datasets, controls, final counts, layout and no network requests | Passed |
| ros-noetic | Linux-hosted Docker; Ubuntu 20.04 / ROS Noetic; rosdep, catkin build/install, original GTest, positive coverage and service contracts, installed-space rostest | Passed |

The ROS catkin tool reported **26 tests, 0 errors, 0 failures, 0 skipped**; this is its aggregate reporting count, not an additional set to sum with nested GTest/rostest results. The original GTest executable ran 11 cases.

The synthetic ROS demo completed with **62 confirmed targets and 8 chunks** in both devel and install-space runs. Its configuration includes inflation and masks; these numbers are not the same experiment as native demo target counts, and are not physical area coverage or speed measurements.

Resolved base image:
`ros:noetic-ros-base-focal@sha256:72b8bc59035dc0a5b8e07aae28c16caa84192971d72d207c72ed734fb1d5e97d`.

## Failed attempts and fixes

| Attempt | Actual outcome | Scoped fix |
|---|---|---|
| [40879911507](https://github.com/lis-y386/ccpp_coverage_planner/actions/runs/40879911507) | Browser passed; native hash check failed; rosdep could not locate tf2 after APT lists were cleared | Emit canonical LF artifact bytes; refresh APT indexes before rosdep installation |
| [40880589216](https://github.com/lis-y386/ccpp_coverage_planner/actions/runs/40880589216) | Native/browser passed; ROS node and Python service/demo tests passed, but original GTest linking failed with undefined main | Link catkin-provided GTEST_MAIN_LIBRARIES in CMake |
| [40881178486](https://github.com/lis-y386/ccpp_coverage_planner/actions/runs/40881178486) | All three jobs passed | Accepted |

The byte-format fix preserves strict SHA256 verification rather than suppressing mismatches. JSON/SVG records are normalized to LF; generated SVG assets use explicit UTF-8 bytes. Hashes were checked against actual Git blobs before pushing. No route coordinates or algorithm behavior were changed.

The GTest fix changes only test linkage. The original test source, core, ROS node, header and service remain unchanged.

## Local synchronization and evidence

Only verified packaging/generation fixes and generated resources were synchronized from the isolated snapshot to this independent project. Documentation and the neutral “see validation” badge were updated locally after the run; they do not pretend that a public main branch has CI.

Local evidence (ignored, retained for review):
- `.maintenance/cloud-ci-20261029/evidence/`: run metadata and failed/successful job logs.
- `.maintenance/cloud-ci-20261029/artifacts/run3/`: downloaded actual Actions artifacts.
- `.maintenance/cloud-ci-20261029/before_cloud_test.zip`: pre-test backup.
- `.maintenance/cloud-ci-20261029/source/`: isolated Git snapshot; its origin points only at the new private test repository.

A machine-readable summary is at [validation/cloud-20261029/result.json](validation/cloud-20261029/result.json). Exact cloud commands are in the committed workflow and successful job log; the Docker command was:
`docker build --progress=plain --target tested -t ccpp-local-test .`.

## Still open

H02/H03 invalid direct-core start and infinite-resolution defects remain reproducible and deliberately unfixed. Existing strict xfails are retained. The complete-window observation and stale-input assumptions remain.

RViz GUI, real point-cloud/TF integration, continuous collision avoidance, flight execution, other hardware/architectures and sanitizer/fuzzing are not established by these tests. Source-rights confirmation and distribution-specific license review remain owner release decisions.

This result is sufficient evidence to describe the tested software snapshot as passing private Linux/Noetic CI. It is not a production-safety or physical-flight certification.
