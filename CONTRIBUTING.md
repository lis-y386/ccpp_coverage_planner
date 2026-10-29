# Contributing

Start with [README](README.md), [algorithm](docs/ALGORITHM.md) and [ROS API](docs/ROS_API.md). A useful issue contains a small map, exact command, parameters, start, expected behavior and actual response. Do not post credentials or irrelevant flight logs.

## Development

Use the standalone build for core work. Original GTest requires a compiler-compatible library or local GoogleTest source. Python tests validate replay records and expose H02/H03 as strict expected failures. See INSTALLATION.md for exact commands.

Run the relevant CTest, pytest, provenance, quality and browser checks. Changes to ROS behavior also need real node/service tests and installed-space validation. Do not claim ROS or flight success from native Windows checks.

## Scope and review

Algorithm and public-interface changes need a separately reviewed design and regressions. This maintenance snapshot deliberately retains upstream core/node/header/service/original tests byte-for-byte. The provenance gate detects changes; an intentional future divergence must be documented against the original baseline, not hidden by rewriting its source record.

clang-format applies to the maintained example, new boundary tests and installed consumer, not preserved upstream files. This scope prevents cosmetic changes from obscuring algorithm review.

## Pull requests

Explain the trigger, resulting behavior, exact tests and limitations. Include an updated minimal example if behavior changes. Generated HTML/GIF/plots must derive from actual C++ JSON and be regenerated with the committed scripts. Keep data provenance and distinguish planned paths, simulated arrivals and real robot results.

Use the existing issue/PR templates once a repository is approved. No public repository or contribution channel is created by this local preparation.
