# Add Tracker Export Release Bundle

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `.agent/PLANS.md` from the repository root. It is self-contained so a future maintainer can understand and resume the work from only this file and the current tree.

## Purpose / Big Picture

The tracker exporter now generates PKCalc-compatible data, validates coverage, builds a drop-in overlay, and checks live PKCalc compatibility. The remaining handoff gap is packaging: maintainers need one ignored build artifact that contains the overlay and the proof files showing why it is safe to hand to a PKCalc fork or reviewer.

After this change, a maintainer can run `make tracker-export-bundle` to produce `build/tracker_export/pkcalc_tracker_release_bundle.tar.gz`, then run `make tracker-export-bundle-check` to unpack and validate that archive. The validator must prove the bundle contains the overlay path layout, overlay manifest, coverage report, compatibility contract snapshot, checksum manifest, and verification notes.

## Progress

- [x] (2026-06-06 23:42Z) Inspected the current tracker Makefile targets, overlay generator, coverage audit script, README, `.gitignore`, and dirty worktree.
- [x] (2026-06-06 23:48Z) Added `tools/tracker_export/export_pkcalc_bundle.py`, which creates an ignored release bundle under `build/tracker_export/`.
- [x] (2026-06-06 23:48Z) Added `tools/tracker_export/validate_pkcalc_bundle.py`, which safely unpacks and verifies bundle contents.
- [x] (2026-06-06 23:48Z) Added explicit `make tracker-export-bundle` and `make tracker-export-bundle-check` targets without wiring either into default `make`.
- [x] (2026-06-06 23:48Z) Updated tracker documentation and patch notes.
- [x] (2026-06-06 23:48Z) Ran syntax checks, tracker export/check, coverage check, overlay check, compatibility check, bundle check, and Docker ROM build because the Makefile changed.

## Surprises & Discoveries

- Observation: `build/` is ignored by `.gitignore`, so bundle directories, unpacked validation directories, generated reports, and `.tar.gz` archives can all live under `build/tracker_export/` without creating tracked artifacts.
  Evidence: `.gitignore` contains `build/`.
- Observation: The existing overlay manifest already records commit, dirty flag, source files, overlay paths, and generated counts, but it does not include coverage-report or compatibility-contract checksums.
  Evidence: `tools/tracker_export/export_pkcalc_overlay.py` writes `manifest.json` with `sourceFiles`, `files`, and `counts`; coverage and contract artifacts are separate files.
- Observation: The Docker ROM build can fail if generated host tool binaries were rebuilt outside the container against an incompatible glibc.
  Evidence: The first Docker ROM build failed at `tools/gbafix/gbafix pokeemerald.elf` with `No such file or directory`. Rebuilding generated tools inside `pokeemerald-expansion:builder` with `make -f make_tools.mk clean-tools tools` fixed the build.

## Decision Log

- Decision: Use a `.tar.gz` bundle named `pkcalc_tracker_release_bundle.tar.gz`.
  Rationale: Python's standard library can create and validate tar archives without adding dependencies, and the archive preserves nested PKCalc overlay paths cleanly.
  Date/Author: 2026-06-06 / Codex.
- Decision: Place bundle contents under a single root directory named `pkcalc_tracker_release_bundle/`.
  Rationale: A single archive root makes unpacking predictable and prevents loose files from scattering into a validation or release directory.
  Date/Author: 2026-06-06 / Codex.
- Decision: Include a `bundle_manifest.json` with SHA-256 checksums for every bundled payload file except the manifest itself.
  Rationale: Counts prove semantic coverage, while checksums prove the exact payload files in the archive match the generated verification metadata. Excluding the manifest avoids a self-referential checksum.
  Date/Author: 2026-06-06 / Codex.

## Outcomes & Retrospective

Implemented the auditable tracker export release bundle. Maintainers now have `make tracker-export-bundle`, which creates `build/tracker_export/pkcalc_tracker_release_bundle.tar.gz`, and `make tracker-export-bundle-check`, which safely unpacks the archive and verifies overlay paths, overlay manifest counts, coverage report, compatibility contract snapshot, SHA-256 checksums, and `VERIFY.txt`. The targeted tracker checks, live compatibility check, bundle check, and Docker ROM build all passed after rebuilding generated tools inside the builder image.

## Context and Orientation

The tracker tools live under `tools/tracker_export/`. Generated tracker artifacts live under `build/tracker_export/`, which is ignored. The overlay generator creates `build/tracker_export/pkcalc_overlay/` with PKCalc-relative files at `js/data/party_order.js`, `js/data/sets.js`, and `js/data/dex/locations.js`. The coverage audit writes `build/tracker_export/coverage_report.json`. The compatibility contract source is committed at `tools/tracker_export/pkcalc_compat_contract.json`.

A release bundle in this plan is not a published release. It is a local ignored archive that packages the generated PKCalc overlay plus proof artifacts for review or handoff. It must not vendor PKCalc source, host anything, expand the tracker data model, or add any default-build behavior.

## Plan of Work

Add `tools/tracker_export/export_pkcalc_bundle.py`. It will accept `--tracker-dir`, `--overlay-dir`, `--coverage-report`, `--contract`, `--bundle-dir`, and `--bundle`. It will copy the overlay directory, coverage report, and compatibility contract into a temporary bundle root under `build/tracker_export/release_bundle/pkcalc_tracker_release_bundle/`, write `VERIFY.txt`, compute SHA-256 checksums for all included files, write `bundle_manifest.json`, and create `build/tracker_export/pkcalc_tracker_release_bundle.tar.gz`.

Add `tools/tracker_export/validate_pkcalc_bundle.py`. It will accept `--bundle` and `--work-dir`, safely unpack the archive under `build/tracker_export/bundle_check/`, verify there is exactly one expected root directory, verify the PKCalc overlay paths exist, verify `VERIFY.txt` exists and names the validation commands, verify the coverage report has `failures: []`, verify the compatibility contract has schema version `1`, verify overlay manifest counts match coverage counts where the two overlap, and verify every checksum in `bundle_manifest.json`.

Update `Makefile` with `TRACKER_BUNDLE_DIR`, `TRACKER_BUNDLE_ARCHIVE`, `TRACKER_BUNDLE_CHECK_DIR`, and phony targets `tracker-export-bundle` and `tracker-export-bundle-check`. The bundle target should depend on `tracker-export-overlay-check` and `tracker-export-coverage-check`. The bundle-check target should depend on the bundle target and run the validator. Do not add either target to default `make`.

Update `tools/tracker_export/README.md` with bundle commands and artifact contents. Update `PATCH_NOTES.md` at the top for the tooling, docs, and planning changes.

## Concrete Steps

From `/home/bayesartre/dev/pokeemerald-expansion-shared-power`, run:

    python3 -m py_compile tools/tracker_export/export_pkcalc_bundle.py tools/tracker_export/validate_pkcalc_bundle.py
    make tracker-export-check
    make tracker-export-coverage-check
    make tracker-export-overlay-check
    make tracker-export-compat-check
    make tracker-export-bundle
    make tracker-export-bundle-check
    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1

If the Docker build fails because host-built tool binaries do not run inside the container, rebuild generated tools inside the container with:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -f make_tools.mk clean-tools tools

Then rerun the Docker ROM build.

## Validation and Acceptance

Acceptance requires `make tracker-export-bundle` to create `build/tracker_export/pkcalc_tracker_release_bundle.tar.gz`. Acceptance also requires `make tracker-export-bundle-check` to unpack that archive and verify overlay paths, overlay manifest, coverage report, compatibility contract snapshot, checksum manifest, and generated verification notes.

The existing tracker gates must still pass: `make tracker-export-check`, `make tracker-export-coverage-check`, `make tracker-export-overlay-check`, and `make tracker-export-compat-check`. Because the Makefile changes, the Docker ROM build with `NO_MULTIBOOT=1` must pass. Generated bundle and unpacked validation files must remain under ignored `build/` paths.

## Idempotence and Recovery

The bundle generator is safe to rerun. It deletes and recreates only the configured bundle staging directory, then overwrites the configured `.tar.gz` archive. The validator is safe to rerun. It deletes and recreates only the configured unpack work directory. If validation fails, regenerate the tracker export, coverage report, overlay, and bundle in that order using the Makefile targets.

## Artifacts and Notes

Syntax checks succeeded:

    python3 -m py_compile tools/tracker_export/export_pkcalc_bundle.py tools/tracker_export/validate_pkcalc_bundle.py tools/tracker_export/audit_tracker_export_coverage.py
    node --check tools/tracker_export/site_smoke_tracker_export_playwright.cjs
    [no output, exit 0]

Tracker export validation succeeded:

    make tracker-export-check
    Validated build/tracker_export/tracker_data.json
    Spot checks passed: TRAINER_SAWYER_1, Route 101 encounters, Route 101 coords

Coverage validation succeeded:

    make tracker-export-coverage-check
    Trainer coverage: 855/855 trainers, 854/854 parties, 1838/1838 sets, 1 skipped empty-party trainer(s)
    Location coverage: 124 map-backed wild header(s), 1975/1975 encounter slots, 2 skipped non-map group(s), 11 skipped unsupported header(s)
    Coverage audit passed: no missing or unexpected tracker records

Overlay validation succeeded:

    make tracker-export-overlay-check
    Validated build/tracker_export/pkcalc_overlay/manifest.json
    Overlay checks passed: file paths, constants, manifest, counts, sources

Live compatibility validation succeeded:

    make tracker-export-compat-check
    "status": "ok"
    "failedAssumptions": []

Bundle generation succeeded:

    make tracker-export-bundle
    Wrote build/tracker_export/release_bundle/pkcalc_tracker_release_bundle
    Wrote build/tracker_export/pkcalc_tracker_release_bundle.tar.gz

Bundle validation succeeded:

    make tracker-export-bundle-check
    Validated build/tracker_export/pkcalc_tracker_release_bundle.tar.gz
    Unpacked build/tracker_export/bundle_check/pkcalc_tracker_release_bundle
    Bundle checks passed: overlay paths, manifest, coverage, contract, checksums, VERIFY.txt

Archive listing confirmed the expected handoff files:

    pkcalc_tracker_release_bundle/
    pkcalc_tracker_release_bundle/VERIFY.txt
    pkcalc_tracker_release_bundle/bundle_manifest.json
    pkcalc_tracker_release_bundle/coverage_report.json
    pkcalc_tracker_release_bundle/pkcalc_compat_contract.json
    pkcalc_tracker_release_bundle/pkcalc_overlay/js/data/dex/locations.js
    pkcalc_tracker_release_bundle/pkcalc_overlay/js/data/party_order.js
    pkcalc_tracker_release_bundle/pkcalc_overlay/js/data/sets.js
    pkcalc_tracker_release_bundle/pkcalc_overlay/manifest.json

The first Docker ROM build failed because `tools/gbafix/gbafix` had been built against a host glibc that did not run inside the container. Rebuilding generated tools inside the builder fixed it:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -f make_tools.mk clean-tools tools
    cc gbafix.c -o gbafix

The required Docker ROM build then succeeded:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    arm-none-eabi-objcopy -O binary pokeemerald.elf pokeemerald.gba
    tools/gbafix/gbafix pokeemerald.gba -p --silent

## Interfaces and Dependencies

Both bundle scripts must use only Python standard-library modules. `export_pkcalc_bundle.py` should use `tarfile` for archive creation and `hashlib.sha256` for checksums. `validate_pkcalc_bundle.py` should use safe tar extraction that rejects absolute paths or `..` path traversal before unpacking.

The bundle root is `pkcalc_tracker_release_bundle/`. Expected files under that root are `pkcalc_overlay/js/data/party_order.js`, `pkcalc_overlay/js/data/sets.js`, `pkcalc_overlay/js/data/dex/locations.js`, `pkcalc_overlay/manifest.json`, `pkcalc_overlay/README.txt`, `coverage_report.json`, `pkcalc_compat_contract.json`, `bundle_manifest.json`, and `VERIFY.txt`.

Revision Note (2026-06-06): Initial ExecPlan for tracker export release bundle implementation.

Revision Note (2026-06-06): Recorded implementation decisions, validation evidence, generated-tool Docker recovery, and final outcome.
