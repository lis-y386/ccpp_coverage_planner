# Release readiness

Assessment: 2026-10-26. **Local engineering/demo preparation is Ready within the validated scope. Public release is Blocked; ROS runtime is Not Verified.** No repository or release was created.

## Ready

| Item | Evidence |
|---|---|
| Recoverable pre-edit baseline | Verified 36-file ZIP and baseline manifest in ignored .maintenance/backups |
| Original algorithm and API preserved | Five upstream-protected file hashes match 8bc54d2 |
| Standalone reproducibility | Native build, 14 CTest entries and 11 original GTest cases executed |
| Honest defect reporting | 27 Python passes + 2 strict known-defect xfails |
| Library installation | Exported ccpp::grid_planner used by an independently configured consumer |
| Real demonstration output | Six actual C++ datasets, semantic checks, SHA256, browser assertions and generated GIF/SVG |
| Documentation/community | English/Chinese homepages, API/algorithm/architecture/install/examples, issue/PR/conduct/security/citation files |
| Local source quality | Scoped clang-format and actual Cppcheck analysis; preserved source not cosmetically rewritten |
| No remote side effects | Local-only directory preparation; no push, release, repository or Pages deployment |

## Blocked

| Gate | Reason / required action |
|---|---|
| Public-source provenance confirmation | Owner must confirm rights to upstream code and any incorporated material; hashes are not authorship proof |
| Supported arbitrary direct-core inputs | H02/H03 remain open by instruction. Approve fixes separately or explicitly restrict an experimental release input contract |
| Claiming a verified ROS package release | Run Noetic build, node/service tests, install-space tests and RViz on the target environment first |
| Binary/container distribution | Review actual resolved dependencies, including xmlrpcpp LGPL-2.1, and preserve required notices |
| Official release identity | Owner must approve repository, metadata, version and future publication; none exists yet |

## Not Verified

- Ubuntu 20.04 / ROS Noetic runtime, rosdep/APT/image availability.
- Devel and installed-space ROS tests, RViz and hardware/simulator behavior.
- Docker build/runtime, pinned resolved digest and package inventory.
- Remote CI, static hosting, external fresh-user clone/download instructions.
- Linux/macOS/MSVC/ARM support, fuzz/sanitizer results and sensor-footprint coverage.

No fictional build-passing badge, Stars count, hosted demo, DOI, paper or institution is used.

## Before-publication checklist

- [ ] Confirm upstream authorship and third-party origin.
- [ ] Decide how H02/H03 will be fixed or constrained; keep regressions and documentation truthful.
- [ ] Execute the tested Docker stage and Noetic devel/install-space tests; retain logs.
- [ ] Inspect RViz output, TF/map semantics and failure cases on target data.
- [ ] Record actual image digest and distribution notices before redistributing binaries/images.
- [ ] Review .gitignore/.dockerignore and export only the source manifest; exclude .maintenance, build directories, backups and caches.
- [ ] Approve repository identity, then run remote CI before changing the CI badge.
- [ ] Approve Pages deployment separately; docs is static-ready but unpublished.
- [ ] Review LICENSE, CITATION, changelog and snapshot identifiers.
- [ ] Only then approve a release/tag; no publishing command is part of this work.

## Suggested GitHub metadata (proposals only)

- Repository: **ccpp_coverage_planner** (matches the ROS package and find_package identity).
- Description: **Standalone C++14 and ROS1 coverage planning with boustrophedon sweeps, A* connections, explicit arrival confirmation, and reproducible grid demos.**
- Topics: robotics, ros, ros-noetic, coverage-path-planning, path-planning, boustrophedon, astar, cpp14, occupancy-grid, visualization.
- First public version: **v0.1.0**, after gates are satisfied. It communicates an early API with known limits and matches package.xml; do not label this snapshot v1.0 or claim production readiness. Until approved, keep CHANGELOG Unreleased and omit invented release dates/DOIs.
- Demo location after explicit hosting approval: repository docs folder as a static site; no guessed URL is supplied.

## Handoff

See DELIVERY.md for the final directory tree, complete added/modified file list, before/after comparison and remaining target-environment verification. See VALIDATION.md and validation/current for actual commands and results.
