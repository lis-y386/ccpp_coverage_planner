# Engineering audit

Audit started 2026-10-25; acceptance reviewed 2026-10-26 (Asia/Shanghai). Baseline: 36 files, no Git metadata, upstream `8bc54d2bdb257b3f2f357b1709350e28d25f35cc`. Every baseline file was archived and checksum-verified before edits; see `BACKUP_AND_RESTORE.md`. The final inventory and changes are in `DELIVERY.md`.

This report distinguishes defects, operational assumptions and missing validation. **No core algorithm, ROS node, public header or service schema is changed in this maintenance pass.** Fixed packaging issues do not resolve algorithm findings.

## Critical

No confirmed Critical finding in this source-level review. This is not a safety certification, security audit or proof of correctness for arbitrary inputs.

## High

| ID / location | Finding and impact | Action / verification | Final disposition |
|---|---|---|---|
| H01 — Dockerfile, CI, ROS node | ROS/catkin, service integration and installed-space operation had no executed evidence on the current Windows machine. Runtime and packaging regressions could remain. | Add explicit devel/install and negative service tests; run Noetic container on a Linux Docker host. Preserve logs. | **Not Verified** until a real ROS run succeeds; no passing badge. |
| H02 — src/grid_planner.cpp: GridPlanner::plan / clearSegment | Core start validation does not independently reject occupied or just-outside starts before snapping to the lattice. The segment sampler omits the source when distance is nonzero. Direct library callers can receive a plan for an invalid start. The ROS node does reject a non-FREE/out-of-bounds start first. | Reproduce in an executable probe; keep a strict expected-failure contract test. Future fix needs separate algorithm approval. | **Open / Blocked for arbitrary untrusted core inputs**; caller precondition documented. |
| H03 — src/grid_planner.cpp: Grid::valid; ROS loadParameters | Numeric validation is incomplete: the core uses resolution > 0 without an isfinite check; grid dimensions multiply as signed int. ROS numeric checks likewise do not establish finite/bounded geometry. | Execute an infinity-resolution probe; document bounded finite inputs. Overflow/fuzz/sanitizer coverage remains future work. | **Open**; no silent core changes. |
| H04 — src/ccpp_planner_node.cpp: cloudCallback / currentPosition | The local rectangular window is assumed fully observed and absent obstacle returns become FREE. There is no input-age rejection or sensor visibility/occlusion model. Feeding a raw sparse cloud or stale odometry can yield misleading FREE space. | Prominent input contract; future stale-input and occlusion tests in simulation. | **Documented assumption; operational validation required**. |
| H05 — license provenance | package.xml declares BSD-3-Clause and this extraction includes that license, but the full historical authorship/third-party origin of upstream code is not established by file hashes. | Inventory bundled/generated assets; preserve original files; owner confirms rights before public release. Do not claim a legal clearance. | **Public release blocked on owner provenance confirmation**. |

## Medium

| ID / location | Finding and impact | Fix / verification | Final disposition |
|---|---|---|---|
| M01 — README / platform support | Noetic target had no end-of-life context or compatibility matrix. A reader might assume ongoing upstream ROS support. | Document Noetic/Focal as a legacy target and link official support dates; keep standalone modern-host CI separate. | Fixed in docs; ROS binary repositories/images still require live container verification. |
| M02 — docs/demo.html / tools/make_preview.py | Only one replay scenario; no speed control; input checks did not validate geometry, totals or unique target acknowledgments. | Multi-scene embedded replay, speed/seek/restart, semantic replay validator, Python/browser tests. | Implemented; final executed evidence in VALIDATION.md. |
| M03 — standalone CMake / tests | Six checks exercised synthetic scenarios, but no dedicated boundary executable, known-defect contracts, style gate or static-analysis record. | Add native boundary tests, strict xfails for known defects, opt-in upstream GTest source build, scoped formatting and cppcheck. | Implemented; supported checks recorded, unsupported checks remain explicit. |
| M04 — install rules | README links referenced resources not all installed together (examples/tools/assets); standalone library had no install/export config. | Install a usable documentation/resources tree and exported standalone target; compile an installed consumer. | Packaging fixed; catkin install-space remains Not Verified locally. |
| M05 — ROS tests | Positive demo test alone could miss no-map, no-odom, invalid-start, type alignment and planning-without-confirmation failures. | Add isolated service-contract rostest without touching the existing test; assert payload fields and state. | Added; Not Verified without ROS runtime. |
| M06 — CI/Docker | ROS dependencies were manually duplicated and container only built; no install-space smoke, timeout, artifacts or least-privilege permissions. | Docker rosdep install, image test/install stages, Linux-host Docker CI with logs and timeouts; no publishing workflow. | Prepared; image availability and actual CI result Not Verified. |
| M07 — absolute ROS names | /ccpp/* are fixed absolute names; normal namespaces alone do not isolate multiple robots. | Preserve API, document separate ROS master/remaps. No interface rename. | Documented; unchanged by request. |
| M08 — reproducibility | No durable multi-scene data manifest, changed-file list or machine-readable validation matrix. | Record source/data hashes, explicit command results, file changes and release gates. | Implemented. |

## Low

| ID / location | Finding and impact | Fix / verification | Final disposition |
|---|---|---|---|
| L01 — README / community files | Chinese-only entry page, no citation, issue/PR templates, security or conduct policy. | English homepage and Chinese translation; original community policies and honest software citation metadata. | Implemented. |
| L02 — ROS node | Header comments/defaults retain upstream integration context, including MAVROS topic strings and an unused heading parameter. | Explain configurable defaults and reserved heading; do not delete necessary upstream code. | Documented; protected source retained. |
| L03 — source formatting | Upstream style differs from new tooling style; automatic formatting would change protected source hashes. | Format/check maintained example and new tests only; list preserved upstream files explicitly. | Scoped gate; legacy formatting debt recorded. |
| L04 — repository identity | Directory is not a Git repository; no independent public URL, CI result, release or DOI exists. | Use local factual badges and relative demo links; suggest metadata in RELEASE_READINESS.md. | No fictional links/claims; repository creation remains user-controlled. |

## Review method and scope

All baseline files were inventoried with SHA256; source, build metadata, service, launch/YAML/RViz, tools, tests, CI, container, docs and generated replay data were inspected. Static checks scan maintained files for machine-specific absolute paths and accidental flight-engine dependencies. Standard input-topic defaults in preserved upstream code are not link-time dependencies. The core includes only its own header and the C++ standard library; the ROS adapter uses declared ROS message/TF/XMLRPC APIs. There is no EGO/MAVROS/PX4/YOLO package linkage.

Potential optimizations (A* allocations, start snapping, finite/range checks, resolution/footprint planning and dynamic input-age handling) are recorded in ROADMAP.md, not applied.

No public repository, push, release, Pages deployment or remote CI was created. The only write scope is this independent directory, including its ignored local backup/tool/build area.

Official support context: [ROS REP-3](https://github.com/ros-infrastructure/rep/blob/master/rep-0003.rst) and [Noetic EOL announcement](https://discourse.ros.org/t/ros-noetic-end-of-life-may-31-2025/43160). Review does not imply full dependency-license clearance; see THIRD_PARTY.md.
