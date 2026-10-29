# Roadmap

These are proposals, not implemented functions. Algorithm/API changes need separate approval.

## Before public v0.1.0

- Owner confirms upstream authorship/rights and dependency distribution obligations.
- Run the Noetic container, devel tests, install-space tests and RViz on Linux; record resolved image digest and packages.
- Resolve or explicitly restrict H02/H03 input-contract defects through a separate approved change.
- Review actual remote CI after a repository is approved.
- Exclude local backups, tool/vendor directories and build products from source publication.

## Input hardening proposal

Reject illegal starts before snapping, reject nonfinite/oversized grids, define mismatched visited-mask behavior, and add Linux sanitizer/fuzz tests. None of these is applied to the preserved core here.

## Runtime robustness proposal

Define stale-input handling, visibility-aware free-space adapters, execution-time path checks, interruption/recovery behavior and distance/point budgets. Validate in simulation and then on appropriate hardware. Short target chunks alone do not establish these properties.

## Research and optimization proposal

Establish map/stride-specific baselines for target coverage, transit repeats and route length before optimizing A* allocations or target order. Assess footprint-aware spacing and corridor connectivity. There are no new performance or novelty claims.

## Platforms and maintenance

Consider a separate ROS2 adapter because Noetic is EOL. Add ARM/MSVC/macOS support only after actual testing. Keep provenance, issue reproduction, command evidence and release gates current. No repository, Pages site or release is published by this task.
