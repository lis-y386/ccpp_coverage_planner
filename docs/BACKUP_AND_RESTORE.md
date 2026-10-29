# Backup and restore

Before this maintenance pass, all 36 files were copied into a ZIP and verified against SHA256. The directory had no Git metadata.

Local-only backup: `.maintenance/backups/before_upgrade_20261025_213825.zip`.
Manifest: `.maintenance/backups/baseline.json`.
The archive includes hidden baseline files. It excludes itself because it was created after inventory capture. Keep `.maintenance/` out of public commits and Docker build contexts.

Safe recovery: extract the ZIP into a **new empty directory**, verify each extracted file against the manifest, then compare it with this directory. Do not extract over a flight workspace. Restoration is a manual owner action; this maintenance pass does not overwrite or delete any existing project.

`docs/upstream_manifest.json` is a separate upstream provenance record, not a backup of this maintenance baseline.
