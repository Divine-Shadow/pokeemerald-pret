# Add Tracker Export Reference Integrity Audit

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `.agent/PLANS.md` from the repository root. It is self-contained so a future maintainer can understand and resume the work from only this file and the current tree.

## Purpose / Big Picture

The tracker exporter now generates, validates, coverage-audits, overlays, live-smokes, and bundles PKCalc-style tracker data. The remaining static-data integrity gap is reference quality: maintainers need a generated report that lists every species, move, ability, item, nature, type, and encounter species reference the overlay contains, and fails if those references are empty, malformed, internally conflicting, or inconsistent with source-derived expectations.

After this change, a maintainer can run `make tracker-export-reference-check` to write `build/tracker_export/reference_report.json`. The release bundle must include that report and `make tracker-export-bundle-check` must validate it.

## Progress

- [x] (2026-06-06 23:53Z) Inspected current tracker Makefile targets, bundle generator, bundle validator, README, and dirty worktree.
- [x] (2026-06-06 23:53Z) Chose generated-data plus source-derived parsing as the validation source for this slice.
- [x] (2026-06-07 00:01Z) Added `tools/tracker_export/audit_tracker_export_references.py`, which writes `build/tracker_export/reference_report.json`.
- [x] (2026-06-07 00:01Z) Added explicit `make tracker-export-reference-check` wiring without adding it to default `make`.
- [x] (2026-06-07 00:01Z) Included the reference report in bundle generation and bundle validation.
- [x] (2026-06-07 00:01Z) Updated tracker documentation and patch notes.
- [x] (2026-06-07 00:01Z) Ran syntax checks, tracker export/check, coverage check, reference check, overlay check, bundle check, compatibility check, and Docker ROM build because the Makefile changed.

## Surprises & Discoveries

- Observation: The current generated tracker data has no exported type references.
  Evidence: Scanning generated `tracker_data.json` showed 0 `teraType` values while trainer set categories include 233 species, 267 moves, 81 abilities, 48 items, and 17 natures.
- Observation: Encounter species are currently lower-case PKCalc-style ids, while trainer set species are display names.
  Evidence: Generated setdex species include values such as `Abra`; generated location encounter species include values such as `abra`.
- Observation: The first reference audit found a real normalized move-name collision between `Double Edge` and `Double-Edge`.
  Evidence: The failed report had `moves` collision `normalized: "doubleedge"` with values `Double Edge` and `Double-Edge`; trainer source contained both spellings.
- Observation: The Docker ROM build can fail if generated host tool binaries were rebuilt outside the container against an incompatible glibc.
  Evidence: The first Docker ROM build failed at `tools/gbafix/gbafix pokeemerald.elf` with `No such file or directory`. Rebuilding generated tools inside `pokeemerald-expansion:builder` with `make -f make_tools.mk clean-tools tools` fixed the build.

## Decision Log

- Decision: Make the reference audit generated-data plus source-derived instead of live-PKCalc-assisted.
  Rationale: The requested slice is still static tracker infrastructure. Source-derived validation can prove the generated references match the repo inputs and can catch empty, malformed, duplicated, or conflicting names without vendoring PKCalc or expanding into full canonical Dex exports.
  Date/Author: 2026-06-06 / Codex.
- Decision: Report type references even when the category is empty.
  Rationale: The contract explicitly names types, and an empty category is useful evidence that the current static tracker export contains no type references yet.
  Date/Author: 2026-06-06 / Codex.
- Decision: Canonicalize `Double Edge` to `Double-Edge` during tracker export display-name normalization.
  Rationale: The source data used both spellings for the same move. PKCalc-style tracker data should not emit two display references that normalize to the same key, so the exporter now chooses the hyphenated move name consistently.
  Date/Author: 2026-06-07 / Codex.

## Outcomes & Retrospective

Implemented tracker export reference-integrity auditing. Maintainers now have `make tracker-export-reference-check`, which writes `build/tracker_export/reference_report.json`, reports species, moves, abilities, items, natures, types, and encounter species, and fails on malformed, missing, unexpected, or normalized-conflicting references. The release bundle now includes and validates `reference_report.json`. The targeted tracker checks, bundle check, live compatibility check, and Docker ROM build all passed after rebuilding generated tools inside the builder image.

## Context and Orientation

The tracker exporter lives under `tools/tracker_export/`. `export_tracker_data.py` parses `src/data/trainers.party` and `src/data/wild_encounters.json`, then writes `build/tracker_export/tracker_data.json`, generated PKCalc JavaScript constants, and overlay artifacts. `audit_tracker_export_coverage.py` already proves all intended trainer parties and map-backed encounter slots are exported.

Reference integrity in this plan means the generated data contains coherent reference strings: trainer species keys, party-order species, moves, abilities, items, natures, optional tera types, and wild encounter species ids. The audit should not claim a reference is accepted by PKCalc's full Dex database unless it has a source for that claim. This slice validates against generated data shape and source-derived expectations.

## Plan of Work

Add `tools/tracker_export/audit_tracker_export_references.py`. It will accept `--output-dir` and `--report`, defaulting to `build/tracker_export` and `build/tracker_export/reference_report.json`. It will load `tracker_data.json`, collect reference categories from `setdex`, `partyOrder`, and `locations`, derive expected references by calling exporter parsing helpers, compare generated categories against those expectations, detect empty or malformed references, detect duplicate/conflicting normalized names within each category, write a JSON report, print concise counts, and exit nonzero on failures.

Update `Makefile` with `TRACKER_REFERENCE_REPORT` and a phony `tracker-export-reference-check` target depending on `tracker-export-check`. Do not add it to default `make`.

Update `tools/tracker_export/export_pkcalc_bundle.py` to accept and copy `reference_report.json`, include it in `bundle_manifest.json`, mention it in `VERIFY.txt`, and depend on `tracker-export-reference-check` through the Makefile target. Update `tools/tracker_export/validate_pkcalc_bundle.py` to require the reference report, assert it has schema version `1` and `failures: []`, verify its counts in the bundle manifest, and require `VERIFY.txt` to mention the reference check command.

Update `tools/tracker_export/README.md` and `PATCH_NOTES.md`.

## Concrete Steps

From `/home/bayesartre/dev/pokeemerald-expansion-shared-power`, run:

    python3 -m py_compile tools/tracker_export/audit_tracker_export_references.py tools/tracker_export/export_pkcalc_bundle.py tools/tracker_export/validate_pkcalc_bundle.py
    make tracker-export-check
    make tracker-export-coverage-check
    make tracker-export-reference-check
    make tracker-export-overlay-check
    make tracker-export-bundle-check
    make tracker-export-compat-check
    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1

If the Docker build fails because generated host tool binaries do not run inside the container, rebuild generated tools inside the container with:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -f make_tools.mk clean-tools tools

Then rerun the Docker ROM build.

## Validation and Acceptance

Acceptance requires `make tracker-export-reference-check` to pass and write `build/tracker_export/reference_report.json`. The report must include categories for species, moves, abilities, items, natures, types, and encounter species, along with counts, values, normalized collision groups, source comparison summaries, and an empty `failures` list.

Acceptance also requires `make tracker-export-bundle-check` to validate that the release bundle includes the reference report and that it has no failures. Existing tracker gates must continue passing: export/check, coverage check, overlay check, and compatibility check. Because the Makefile changes, the Docker ROM build with `NO_MULTIBOOT=1` must pass.

## Idempotence and Recovery

The reference audit is safe to rerun. It reads generated tracker data and source files, then overwrites only the configured report path under `build/tracker_export/` by default. Bundle generation and validation remain safe to rerun under ignored `build/` paths. If reference failures appear after source changes, inspect `reference_report.json`, fix the exporter or the source data, and rerun the Makefile targets.

## Artifacts and Notes

Syntax checks succeeded:

    python3 -m py_compile tools/tracker_export/audit_tracker_export_references.py tools/tracker_export/export_tracker_data.py tools/tracker_export/export_pkcalc_bundle.py tools/tracker_export/validate_pkcalc_bundle.py tools/tracker_export/audit_tracker_export_coverage.py
    node --check tools/tracker_export/site_smoke_tracker_export_playwright.cjs
    [no output, exit 0]

Tracker export succeeded:

    make tracker-export
    Wrote build/tracker_export/tracker_data.json
    Wrote build/tracker_export/pkcalc/sets.js
    Wrote build/tracker_export/pkcalc/party_order.js
    Wrote build/tracker_export/pkcalc/locations.js

Tracker export validation succeeded:

    make tracker-export-check
    Validated build/tracker_export/tracker_data.json
    Spot checks passed: TRAINER_SAWYER_1, Route 101 encounters, Route 101 coords

Coverage validation succeeded:

    make tracker-export-coverage-check
    Trainer coverage: 855/855 trainers, 854/854 parties, 1838/1838 sets, 1 skipped empty-party trainer(s)
    Location coverage: 124 map-backed wild header(s), 1975/1975 encounter slots, 2 skipped non-map group(s), 11 skipped unsupported header(s)
    Coverage audit passed: no missing or unexpected tracker records

Reference validation succeeded:

    make tracker-export-reference-check
    Reference categories: species: 233, moves: 266, abilities: 81, items: 48, natures: 17, types: 0, encounterSpecies: 132
    Reference audit passed: no malformed, missing, unexpected, or conflicting references

The generated reference report has no failures or collisions:

    failures 0
    species 233 2071 233 collisions 0
    moves 266 6860 266 collisions 0
    abilities 81 1050 81 collisions 0
    items 48 1483 48 collisions 0
    natures 17 1838 17 collisions 0
    types 0 0 0 collisions 0
    encounterSpecies 132 1975 132 collisions 0

Overlay validation succeeded:

    make tracker-export-overlay-check
    Validated build/tracker_export/pkcalc_overlay/manifest.json
    Overlay checks passed: file paths, constants, manifest, counts, sources

Bundle validation succeeded and now checks references:

    make tracker-export-bundle-check
    Bundle checks passed: overlay paths, manifest, coverage, references, contract, checksums, VERIFY.txt

The bundle includes the reference report:

    pkcalc_tracker_release_bundle/reference_report.json

The bundle manifest records reference report validation:

    referenceReport reference_report.json
    referenceFailures 0
    moves {'count': 266, 'occurrences': 6860, 'normalizedCount': 266}

Live compatibility validation succeeded:

    make tracker-export-compat-check
    "status": "ok"
    "failedAssumptions": []

The first Docker ROM build failed because `tools/gbafix/gbafix` had been built against a host glibc that did not run inside the container. Rebuilding generated tools inside the builder fixed it:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -f make_tools.mk clean-tools tools
    cc gbafix.c -o gbafix

The required Docker ROM build then succeeded:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    arm-none-eabi-objcopy -O binary pokeemerald.elf pokeemerald.gba
    tools/gbafix/gbafix pokeemerald.gba -p --silent

## Interfaces and Dependencies

`audit_tracker_export_references.py` must use only Python standard-library modules and import `tools/tracker_export/export_tracker_data.py` for source-derived parsing and normalization. The report schema version is `1`.

Revision Note (2026-06-06): Initial ExecPlan for tracker export reference-integrity auditing.

Revision Note (2026-06-07): Recorded implementation decisions, validation evidence, generated-tool Docker recovery, and final outcome.
