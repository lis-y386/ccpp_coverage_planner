# Release readiness

## v0.1.0-alpha.1: experimental source-only scope

The maintainer has explicitly requested repository publication and an alpha prerelease. This supersedes the earlier instruction to keep the validation repository private and unreleased.

**Ready for an experimental source preview with documented input preconditions. Stable / production / flight-ready status remains Blocked.**

| Status | Item | Evidence / scope |
|---|---|---|
| Ready | Recoverable baseline | Local verified backups are retained outside the public payload |
| Ready | Protected source identity | Core, ROS node, public header, service and original test hashes match 8bc54d2 |
| Ready | Native and browser validation | Actual Linux CI and prior local checks; see CLOUD_VALIDATION.md |
| Ready | Headless ROS Noetic | Actual Docker catkin build, original GTest, service/demo and install-space checks |
| Ready | Honest experimental scope | H02/H03, observation assumptions and unverified environments are prominent |
| Ready | Publication authorization | Explicit maintainer request to rename, make public and publish a test version |
| Blocked | Arbitrary-input robustness | H02/H03 remain open; direct callers must enforce finite/bounded geometry and a FREE in-bounds start |
| Blocked | Production/flight-ready release | No continuous execution safety or physical validation has been established |
| Not Verified | RViz GUI / actual sensors / hardware | Headless synthetic tests do not establish these capabilities |
| Not Verified | Other architectures / sanitizers / fuzzing | No results claimed |
| Not Verified | Independent legal/historical authorship audit | Publication is the maintainer's source-release decision; hashes are not ownership proof |

The release procedure must gate publication on the candidate commit's CI, keep it marked Pre-release, and attach only source/demo artifacts. No compiled ROS binaries or registry image are being distributed.

## Publication checks

- Keep the repository named ccpp_coverage_planner.
- Use v0.1.0-alpha.1 and mark it Pre-release, not a stable Latest release.
- Preserve all original core/API/test files.
- Exclude .maintenance, build directories, caches, local backups and machine-specific validation logs.
- Review tracked history and release attachments for credentials and unintended payloads.
- Record the exact candidate SHA, successful CI URL, tag and asset checksums in the local publication report.
- Keep experimental input restrictions and H02/H03 visible in README and Release notes.
- Do not enable Pages or publish container images as part of this source-release task.

See [release notes](releases/v0.1.0-alpha.1.md), [cloud evidence](CLOUD_VALIDATION.md) and [engineering audit](ENGINEERING_AUDIT.md).
