# Add Tracker Export Coverage Auditing

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `.agent/PLANS.md` from the repository root. It is self-contained so a future maintainer can understand and resume the work from only this file and the current tree.

## Purpose / Big Picture

The tracker exporter can generate PKCalc-shaped trainer and wild-location data, and the browser smoke checks prove PKCalc can load those files. The missing operator guarantee is coverage: maintainers need to know that every intended trainer party and every map-backed wild encounter location made it into the generated data, and that any unsupported record was skipped for an explicit reason.

After this change, a maintainer can run a coverage audit command and receive both a concise terminal summary and a generated JSON report under `build/tracker_export/`. The command should fail when source data contains duplicate trainer IDs, malformed records, missing generated trainer entries, missing party-order entries, missing PKCalc set entries, or map-backed wild encounter slots that do not appear in `LOCATIONS`.

## Progress

- [x] (2026-06-06 23:29Z) Inspected the current tracker exporter, validator, Makefile tracker targets, tracker README, and dirty worktree.
- [x] (2026-06-06 23:29Z) Confirmed the current exporter parses 855 trainer sections, 854 non-empty trainer parties, 233 setdex species, 1,838 setdex sets, 213 region-map locations, 99 locations with source maps, 64 locations with encounters, and 1,975 exported encounter slots.
- [x] (2026-06-06 23:37Z) Added `tools/tracker_export/audit_tracker_export_coverage.py`, which writes a report under `build/tracker_export/` by default.
- [x] (2026-06-06 23:37Z) Added explicit `make tracker-export-coverage-check` wiring without adding it to default `make`.
- [x] (2026-06-06 23:37Z) Updated tracker documentation and patch notes.
- [x] (2026-06-06 23:37Z) Ran tracker export/check, coverage audit, overlay check, compatibility check, and Docker ROM build because the Makefile changed.

## Surprises & Discoveries

- Observation: The trainer source has one parsed trainer section with an empty party.
  Evidence: Reusing `parse_trainers` against `src/data/trainers.party` reports 855 trainers and 854 `partyOrder` entries.
- Observation: Most region-map sections do not have map grid coordinates and therefore cannot be audited as map-backed encounter locations.
  Evidence: `src/data/region_map/region_map_sections.json` contains 213 sections, with 118 sections missing one or more of `x`, `y`, `width`, and `height`.
- Observation: `src/data/wild_encounters.json` has two non-map encounter groups for Battle Pyramid and Battle Pike.
  Evidence: The groups named `gBattlePyramidWildMonHeaders` and `gBattlePikeWildMonHeaders` have `for_maps: false` and are outside the map-backed location export scope.
- Observation: A host-built `tools/gbafix/gbafix` binary can fail inside the Docker builder as `No such file or directory` when its glibc requirement is newer than the container's glibc.
  Evidence: The first Docker ROM build failed at `tools/gbafix/gbafix pokeemerald.elf`; running `ldd` inside the container reported `GLIBC_2.38 not found`. Cleaning and rebuilding generated tools inside `pokeemerald-expansion:builder` fixed the build.

## Decision Log

- Decision: Add a separate `tracker-export-coverage-check` target instead of folding coverage into the existing `tracker-export-check`.
  Rationale: The existing validator remains a fast consistency and spot-check command, while the coverage target provides a clear audit artifact and stricter source-to-output coverage checks. The target is still explicit and not wired into default `make`.
  Date/Author: 2026-06-06 / Codex.
- Decision: Write the coverage report as generated JSON under `build/tracker_export/coverage_report.json`.
  Rationale: Coverage data is derived from current source files and generated tracker output, so it belongs with other ignored build artifacts rather than committed source.
  Date/Author: 2026-06-06 / Codex.

## Outcomes & Retrospective

Implemented tracker export coverage auditing. Maintainers now have `make tracker-export-coverage-check`, which regenerates and validates tracker output, writes `build/tracker_export/coverage_report.json`, reports explicit skip reasons, and fails on missing or unexpected trainer/location coverage. The targeted tracker checks, live compatibility check, and Docker ROM build all passed after rebuilding generated tool binaries inside the builder image.

## Context and Orientation

The tracker exporter lives under `tools/tracker_export/`. `export_tracker_data.py` reads `src/data/trainers.party`, `src/data/wild_encounters.json`, `data/maps/*/map.json`, and `src/data/region_map/region_map_sections.json`, then writes generated files under `build/tracker_export/`. Generated files under `build/` are ignored.

Trainer coverage means that every trainer section parsed from `src/data/trainers.party` exists in `tracker_data.json["trainers"]`; every trainer with a non-empty party has a matching `partyOrder` entry keyed by its PKCalc label; and every party Pokemon has a generated PKCalc set under `setdex`, with duplicate species in one party using the exporter’s `#2`, `#3`, and later suffixes.

Location coverage means that every wild encounter header that names a map whose `data/maps/*/map.json` file maps to a region-map section with coordinates contributes its expected encounter slots to `tracker_data.json["locations"][locationId]["encounters"]`. Wild groups that are not map-backed, encounter headers without map JSON, and map sections without coordinates must be reported as explicit skips rather than silently ignored.

## Plan of Work

Add `tools/tracker_export/audit_tracker_export_coverage.py`. The script will accept `--output-dir` and `--report`, defaulting to `build/tracker_export` and `build/tracker_export/coverage_report.json`. It will load `tracker_data.json`, reparse source trainer and wild encounter data through the same normalization helpers as the exporter, compare expected records against the generated output, write a JSON report with counts, skips, and failures, print concise coverage summaries, and exit nonzero when failures exist.

Update `Makefile` by adding `TRACKER_COVERAGE_REPORT` and a phony `tracker-export-coverage-check` target that depends on `tracker-export-check` and runs the audit script. Do not add the target to default `make`.

Update `tools/tracker_export/README.md` with the new command, report path, and skip/failure meaning. Update `PATCH_NOTES.md` at the top for the tooling, docs, and planning changes.

## Concrete Steps

From `/home/bayesartre/dev/pokeemerald-expansion-shared-power`, run:

    python3 -m py_compile tools/tracker_export/audit_tracker_export_coverage.py
    make tracker-export-check
    make tracker-export-coverage-check
    make tracker-export-overlay-check
    make tracker-export-compat-check
    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1

If the site smoke or compatibility smoke scripts are not changed in this slice, the compatibility check still gives a useful end-to-end sanity check that the stricter coverage audit did not break generated PKCalc artifacts.

## Validation and Acceptance

Acceptance requires `make tracker-export-coverage-check` to pass and write `build/tracker_export/coverage_report.json`. The report must include trainer counts, set counts, location counts, supported wild encounter header counts, skipped unsupported records with reasons, and an empty `failures` list. It must fail on duplicate trainer IDs, missing generated trainers, missing party-order entries, missing setdex entries, malformed encounter groups, or missing map-backed encounter slots.

Acceptance also requires the existing tracker export/check and overlay validation to continue passing. Because the Makefile changes, the Docker ROM build with `NO_MULTIBOOT=1` must pass. If the live compatibility target is run, it should still report `status: "ok"` and `failedAssumptions: []`.

## Idempotence and Recovery

The coverage audit is safe to rerun. It reads source files and generated tracker data, then overwrites only the chosen report path under `build/tracker_export/` by default. If a coverage failure appears after source data changes, rerun `make tracker-export-coverage-check` after fixing the exporter or updating the explicit skip taxonomy. Generated data can be reset by deleting `build/tracker_export/` and rerunning the target.

## Artifacts and Notes

Syntax checks succeeded:

    python3 -m py_compile tools/tracker_export/audit_tracker_export_coverage.py
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

Coverage audit succeeded:

    make tracker-export-coverage-check
    Wrote build/tracker_export/coverage_report.json
    Trainer coverage: 855/855 trainers, 854/854 parties, 1838/1838 sets, 1 skipped empty-party trainer(s)
    Location coverage: 124 map-backed wild header(s), 1975/1975 encounter slots, 2 skipped non-map group(s), 11 skipped unsupported header(s)
    Coverage audit passed: no missing or unexpected tracker records

The generated report has no failures and explicit skip reasons:

    "skips": {
      "trainers": [
        { "trainerId": "TRAINER_NONE", "reason": "empty_party_no_pkcalc_sets" }
      ],
      "wildEncounterGroups": [
        { "group": "gBattlePyramidWildMonHeaders", "reason": "non_map_wild_encounter_group", "headers": 7 },
        { "group": "gBattlePikeWildMonHeaders", "reason": "non_map_wild_encounter_group", "headers": 4 }
      ],
      "wildEncounterHeaders": []
    }
    "failures": []

Overlay validation succeeded:

    make tracker-export-overlay-check
    Validated build/tracker_export/pkcalc_overlay/manifest.json
    Overlay checks passed: file paths, constants, manifest, counts, sources

Live compatibility validation succeeded:

    make tracker-export-compat-check
    "status": "ok"
    "failedAssumptions": []

The first Docker ROM build failed because `tools/gbafix/gbafix` had been built against a newer host glibc than the container had. Rebuilding generated tools inside the builder fixed it:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -f make_tools.mk clean-tools tools
    cc gbafix.c -o gbafix

The required Docker ROM build then succeeded:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    arm-none-eabi-objcopy -O binary pokeemerald.elf pokeemerald.gba
    tools/gbafix/gbafix pokeemerald.gba -p --silent

## Interfaces and Dependencies

`audit_tracker_export_coverage.py` depends only on Python standard-library modules and `tools/tracker_export/export_tracker_data.py`. It should import the exporter’s parsing and normalization helpers so coverage expectations use the same IDs and labels as the generated artifact. The report schema version is `1`.

Revision Note (2026-06-06): Initial ExecPlan for tracker export coverage auditing.

Revision Note (2026-06-06): Recorded implementation decisions, validation evidence, generated-tool Docker recovery, and final outcome.
