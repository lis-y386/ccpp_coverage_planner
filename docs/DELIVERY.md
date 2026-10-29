# Delivery and acceptance handoff

Date: 2026-10-26 (Asia/Shanghai). Only this independent project was edited. No original flight workspace, existing project or original GitHub remote was modified. No push, public repository, release or Pages deployment occurred.

## Snapshot / restore

- Upstream: 8bc54d2bdb257b3f2f357b1709350e28d25f35cc.
- Root had no Git metadata before editing and was not initialized or committed.
- Before-edit backup: .maintenance/backups/before_upgrade_20261025_213825.zip; all 36 baseline files verified.
- All original files remain. Five protected upstream sources/API/test files still match their original hashes.
- Recovery steps: BACKUP_AND_RESTORE.md.

## Before / after

| Area | Before | After |
|---|---|---|
| Homepage | Chinese entry + short English page | English primary, Chinese translation, real banner/GIF and factual badges |
| Native tests | 6 synthetic checks, original GTest not run | 14 CTest entries, including 11 actually executed original GTest cases |
| Boundaries | No desired-contract regression record | Dedicated assertions; 27 Python passes and 2 strict known-defect xfails |
| Replay | One scene, play/seek/reset | Six actual C++ scenes, speed/timeline, semantic hashes, browser assertions |
| Installation | No native export/consumer | Exported ccpp::grid_planner; actual installed consumer builds/runs |
| ROS evidence | Initial demo, unverified | Real service tests and installed-space CI prepared; still Not Verified locally |
| Quality | No scoped style/static gate | clang-format and Cppcheck executed; failed tool attempts retained |
| Community | Partial | Contribution/conduct/security/citation, issue and PR templates |
| Provenance | Upstream hashes | Same protected files plus backup, asset/data hashes, source inventory |

## Actual acceptance evidence

All 16 final command groups exited 0. Exact command arrays, process overrides, exit codes and logs: validation/current/final_commands.json. Test durations are execution-log metadata, not algorithm performance claims.

- CTest: 14/14 entries; nested original GTest: 11/11 cases.
- Python: 27 passed; 2 strict expected failures; no actual skip.
- Browser: six exact JSON datasets, final counts, controls, no HTTP requests/page errors; desktop and narrow screenshots inspected.
- GIF: 55 frames, all decoded; final target display matches 292 goals. Illustrative animation timing only.
- Native install/export and separate consumer: passed.
- Scoped formatting/static checks: passed. Original protected source was not reformatted.
- CITATION.cff: official schema validated; report includes schema URL/hash.

## Remaining issues / target validation

H02 invalid direct-core starts and H03 nonfinite resolution remain deliberately unfixed; H04 map-window/stale-input assumptions require integration validation. No algorithm change is authorized by this maintenance pass.

Noetic/catkin devel and installed-space tests, RViz, Docker, remote CI, other platforms, sanitizer/fuzzing and actual robot behavior are Not Verified. Public release is Blocked on the explicit gates in RELEASE_READINESS.md, including owner provenance confirmation and dependency distribution review.

## Recommended future repository metadata

Name: ccpp_coverage_planner. Description, Topics, proposed first v0.1.0 and rationale are in RELEASE_READINESS.md. These are proposals; no repository/version was published.

## Final directory tree (public payload)

```text
ccpp_coverage_planner/
  .clang-format
  .dockerignore
  .gitattributes
  .github/
    ISSUE_TEMPLATE/
      bug_report.yml
      config.yml
      feature_request.yml
    pull_request_template.md
    workflows/
      ci.yml
  .gitignore
  CHANGELOG.md
  CITATION.cff
  CMakeLists.txt
  CODE_OF_CONDUCT.md
  CONTRIBUTING.md
  Dockerfile
  LICENSE
  README.md
  README_EN.md
  README_zh-CN.md
  SECURITY.md
  assets/
    PROVENANCE.json
    architecture.svg
    badges/
      ci.svg
      cpp.svg
      license.svg
      ros.svg
    banner.svg
    demo.gif
    scenes.svg
  cmake/
    ccpp_coverage_plannerConfig.cmake.in
  config/
    ccpp.yaml
  docs/
    .nojekyll
    ALGORITHM.md
    ARCHITECTURE.md
    BACKUP_AND_RESTORE.md
    DELIVERY.md
    ENGINEERING_AUDIT.md
    EXAMPLES.md
    INSTALLATION.md
    INTERFACES.md
    PROVENANCE.md
    RELEASE_READINESS.md
    ROADMAP.md
    ROS_API.md
    TEST_REPORT.md
    THIRD_PARTY.md
    TROUBLESHOOTING.md
    VALIDATION.md
    data/
      disconnected.json
      disconnected.svg
      manifest.json
      maze.json
      maze.svg
      obstacles.json
      obstacles.svg
      rectangle.json
      rectangle.svg
      skip.json
      skip.svg
      stride.json
      stride.svg
    demo.html
    demo_obstacles.json
    demo_obstacles.svg
    index.html
    upstream_manifest.json
    validation/
      current/
        assets.log
        banner.png
        browser.json
        browser.log
        citation.json
        configure.log
        consumer_build.log
        consumer_configure.log
        consumer_run.log
        ctest.log
        demo_desktop.png
        demo_generation.json
        demo_mobile.png
        final_commands.json
        generate.log
        gtest.xml
        initial_0.log
        initial_1.log
        initial_2.log
        initial_commands.json
        install.log
        install_quality_0.log
        install_quality_1.log
        install_quality_2.log
        install_quality_3.log
        install_quality_4.log
        install_quality_commands.json
        known_defects.json
        media.json
        native.xml
        not_verified.json
        original_gtest.log
        preview.log
        project.json
        project.log
        provenance.log
        python.log
        python.xml
        quality.json
        quality.log
        quality_first_attempt.json
        source_inventory.json
      local_checks.json
      native_ctest.log
  examples/
    standalone_demo.cpp
  include/
    ccpp_coverage_planner/
      grid_planner.h
  launch/
    ccpp_planner.launch
    demo.launch
  package-lock.json
  package.json
  package.xml
  pytest.ini
  requirements-dev.txt
  rviz/
    demo.rviz
  scripts/
    demo_world.py
  src/
    ccpp_planner_node.cpp
    grid_planner.cpp
  srv/
    PlanCoverage.srv
  test/
    demo_integration.test
    install_consumer/
      CMakeLists.txt
      main.cpp
    service_contract.test
    test_core_boundaries.cpp
    test_demo_integration.py
    test_grid_planner.cpp
    test_known_contracts.py
    test_service_contract.py
    test_tools.py
  tools/
    check_project.py
    check_quality.py
    demo_template.html
    generate_demo.py
    make_preview.py
    render_assets.py
    replay_data.py
    test_browser.cjs
    verify_provenance.py
```

Local-only, excluded: .maintenance/ (backup, downloaded tool/test sources, installed prefix), build/ and build-consumer/. These are intentionally retained for reproducibility and must not be published.

## Complete modification list

Baseline files modified: 15. Added public files: 107. Removed original files: 0. Full list below.

### Modified baseline files

- .dockerignore
- .gitattributes
- .github/workflows/ci.yml
- .gitignore
- CHANGELOG.md
- CMakeLists.txt
- CONTRIBUTING.md
- Dockerfile
- README.md
- README_EN.md
- docs/PROVENANCE.md
- docs/VALIDATION.md
- docs/demo.html
- examples/standalone_demo.cpp
- tools/make_preview.py

### Added files

- .clang-format
- .github/ISSUE_TEMPLATE/bug_report.yml
- .github/ISSUE_TEMPLATE/config.yml
- .github/ISSUE_TEMPLATE/feature_request.yml
- .github/pull_request_template.md
- CITATION.cff
- CODE_OF_CONDUCT.md
- README_zh-CN.md
- SECURITY.md
- assets/PROVENANCE.json
- assets/architecture.svg
- assets/badges/ci.svg
- assets/badges/cpp.svg
- assets/badges/license.svg
- assets/badges/ros.svg
- assets/banner.svg
- assets/demo.gif
- assets/scenes.svg
- cmake/ccpp_coverage_plannerConfig.cmake.in
- docs/.nojekyll
- docs/ALGORITHM.md
- docs/ARCHITECTURE.md
- docs/BACKUP_AND_RESTORE.md
- docs/DELIVERY.md
- docs/ENGINEERING_AUDIT.md
- docs/EXAMPLES.md
- docs/INSTALLATION.md
- docs/RELEASE_READINESS.md
- docs/ROADMAP.md
- docs/ROS_API.md
- docs/TEST_REPORT.md
- docs/THIRD_PARTY.md
- docs/TROUBLESHOOTING.md
- docs/data/disconnected.json
- docs/data/disconnected.svg
- docs/data/manifest.json
- docs/data/maze.json
- docs/data/maze.svg
- docs/data/obstacles.json
- docs/data/obstacles.svg
- docs/data/rectangle.json
- docs/data/rectangle.svg
- docs/data/skip.json
- docs/data/skip.svg
- docs/data/stride.json
- docs/data/stride.svg
- docs/index.html
- docs/validation/current/assets.log
- docs/validation/current/banner.png
- docs/validation/current/browser.json
- docs/validation/current/browser.log
- docs/validation/current/citation.json
- docs/validation/current/configure.log
- docs/validation/current/consumer_build.log
- docs/validation/current/consumer_configure.log
- docs/validation/current/consumer_run.log
- docs/validation/current/ctest.log
- docs/validation/current/demo_desktop.png
- docs/validation/current/demo_generation.json
- docs/validation/current/demo_mobile.png
- docs/validation/current/final_commands.json
- docs/validation/current/generate.log
- docs/validation/current/gtest.xml
- docs/validation/current/initial_0.log
- docs/validation/current/initial_1.log
- docs/validation/current/initial_2.log
- docs/validation/current/initial_commands.json
- docs/validation/current/install.log
- docs/validation/current/install_quality_0.log
- docs/validation/current/install_quality_1.log
- docs/validation/current/install_quality_2.log
- docs/validation/current/install_quality_3.log
- docs/validation/current/install_quality_4.log
- docs/validation/current/install_quality_commands.json
- docs/validation/current/known_defects.json
- docs/validation/current/media.json
- docs/validation/current/native.xml
- docs/validation/current/not_verified.json
- docs/validation/current/original_gtest.log
- docs/validation/current/preview.log
- docs/validation/current/project.json
- docs/validation/current/project.log
- docs/validation/current/provenance.log
- docs/validation/current/python.log
- docs/validation/current/python.xml
- docs/validation/current/quality.json
- docs/validation/current/quality.log
- docs/validation/current/quality_first_attempt.json
- docs/validation/current/source_inventory.json
- package-lock.json
- package.json
- pytest.ini
- requirements-dev.txt
- test/install_consumer/CMakeLists.txt
- test/install_consumer/main.cpp
- test/service_contract.test
- test/test_core_boundaries.cpp
- test/test_known_contracts.py
- test/test_service_contract.py
- test/test_tools.py
- tools/check_project.py
- tools/check_quality.py
- tools/demo_template.html
- tools/generate_demo.py
- tools/render_assets.py
- tools/replay_data.py
- tools/test_browser.cjs

## Publication checklist

The Ready / Blocked / Not Verified matrix and owner checklist are in RELEASE_READINESS.md. Review local validation log path privacy before public sharing; preserve raw evidence locally and document any redaction. Do not upload .maintenance or build trees.
