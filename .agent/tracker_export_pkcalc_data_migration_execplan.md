# Add the first PKCalc data-migration proof slice

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This document must be maintained in accordance with `.agent/PLANS.md`.

## Purpose / Big Picture

Tracker export work can already route trainer sets and encounters into the live PKCalc app, but broader PKCalc data migration needs a safer first step than replacing full species, moves, items, or abilities. After this change, maintainers can run explicit commands that inspect the live PKCalc catalog shapes, map repository source fields to PKCalc fields, and prove one low-risk replacement catalog generated from this repo matches PKCalc's live expectations.

The first proof category is natures. Natures are small, deterministic, and represented in this repo by `gNaturesInfo` with names and stat modifiers. The proof artifacts will live under `build/tracker_export/data_migration/` and will not be routed into the live overlay by default.

## Progress

- [x] (2026-06-07 00:57Z) Created this ExecPlan and chose natures as the first migration proof after inspecting repo source and live PKCalc nature shapes.
- [x] (2026-06-07 01:07Z) Added a repo-source exporter that generates PKCalc-shaped nature proof artifacts under `build/tracker_export/data_migration/`.
- [x] (2026-06-07 01:11Z) Added a Playwright shape audit and validation script that compares generated nature artifacts against live PKCalc `NATURES_BY_ID` and `calc.NATURES`.
- [x] (2026-06-07 01:12Z) Added explicit Makefile targets for the catalog shape audit and nature migration validation.
- [x] (2026-06-07 01:15Z) Updated tracker documentation and `PATCH_NOTES.md`.
- [x] (2026-06-07 01:18Z) Ran the new migration targets, existing tracker checks, compatibility and bundle checks, and Docker `make NO_MULTIBOOT=1`; Docker passed after rebuilding host tools inside the builder image with `/usr/bin/gcc` and `/usr/bin/g++`.

## Surprises & Discoveries

- Observation: Natures are safer than abilities for the first proof.
  Evidence: `src/pokemon.c` contains `gNaturesInfo[NUM_NATURES]` with `.name`, `.statUp`, and `.statDown` for all 25 natures. Ability data spans names, descriptions, AI ratings, and multiple behavior flags in `src/data/abilities.h` and `include/pokemon.h`, so migrating abilities would imply behavior semantics beyond this slice.

- Observation: Live PKCalc has two relevant nature surfaces.
  Evidence: A Playwright probe showed `NATURES_BY_ID.adamant` has `{kind:"Nature", id:"adamant", name:"Adamant", plus:"atk", minus:"spa"}`, and `calc.NATURES.Adamant` is `["atk", "spa"]`.

- Observation: Live PKCalc uses different shapes for BY_ID catalogs.
  Evidence: The mapping report now samples `SPECIES_BY_ID` as `1.abra` with fields `baseStats`, `id`, `kind`, `name`, `nfe`, `types`, and `weightkg`; `MOVES_BY_ID` as `1.nomove` with `basePower`, `category`, `flags`, `id`, `kind`, `name`, and `type`; `ABILITIES_BY_ID` as `3.airlock` with `id`, `kind`, and `name`; `ITEMS_BY_ID` as `2.berryjuice` with `id`, `kind`, `megaEvolves`, and `name`; and `NATURES_BY_ID` as `adamant` with `id`, `kind`, `minus`, `name`, and `plus`.

- Observation: Docker could not execute host tools that had been rebuilt by the Nix toolchain.
  Evidence: `tools/gbafix/gbafix` initially requested `/nix/store/57iz36553175g3178pvxjij8z5rcsd4n-glibc-2.42-61/lib/ld-linux-x86-64.so.2`, causing Docker `make NO_MULTIBOOT=1` to fail with `tools/gbafix/gbafix: No such file or directory`. Rebuilding tools in Docker with `CC=/usr/bin/gcc CXX=/usr/bin/g++ LDFLAGS=` changed the interpreter to `/lib64/ld-linux-x86-64.so.2`, and the Docker ROM build then passed.

## Decision Log

- Decision: Generate a natures proof catalog rather than an abilities catalog.
  Rationale: Natures have complete source fields and a small live PKCalc schema, while abilities require behavior semantics and descriptions that are better left for a later category-specific migration.
  Date/Author: 2026-06-07 / Codex

- Decision: Keep proof artifacts under `build/tracker_export/data_migration/` and do not add them to the PKCalc overlay.
  Rationale: The goal is a migration proof and shape mapping, not a default data replacement in the live app handoff path.
  Date/Author: 2026-06-07 / Codex

## Outcomes & Retrospective

Completed. The repository now has an opt-in PKCalc data-migration proof path that audits live catalog shapes and generates a natures replacement catalog from `gNaturesInfo`. Natures were selected because their source data and PKCalc schemas are complete and low risk; broader species, move, item, and ability migration remains intentionally deferred.

The generated reports show `chosenMigrationCategory` as `natures`, no missing fields or incompatible values for the chosen category, 25 source natures, 25 PKCalc `NATURES_BY_ID` entries, and 25 `calc.NATURES` entries. The proof artifacts remain under `build/tracker_export/data_migration/` and are not wired into default overlay export behavior.

## Context and Orientation

The tracker tooling lives in `tools/tracker_export/`. Existing Makefile targets generate tracker data under `build/tracker_export/`, route an overlay into live PKCalc, validate static references, run a live reference audit, and package release bundles. This slice adds a separate data-migration proof path under `build/tracker_export/data_migration/`.

PKCalc exposes a live nature catalog named `NATURES_BY_ID`, keyed by lowercase ids such as `adamant`, where each value has `kind`, `id`, `name`, `plus`, and `minus`. PKCalc also exposes a calculator nature map at `calc.NATURES`, keyed by display name, where each value is a two-element array `[plus, minus]`. The stat names PKCalc uses are `atk`, `def`, `spa`, `spd`, and `spe`.

The repository source of truth for natures is `src/pokemon.c`, in the `gNaturesInfo[NUM_NATURES]` array. The numeric nature constants and stat constants are in `include/constants/pokemon.h`. Repository stat constants map to PKCalc fields as `STAT_ATK -> atk`, `STAT_DEF -> def`, `STAT_SPEED -> spe`, `STAT_SPATK -> spa`, and `STAT_SPDEF -> spd`.

## Plan of Work

Add a Python exporter, `tools/tracker_export/export_pkcalc_data_migration.py`, that parses `include/constants/pokemon.h` and `src/pokemon.c`, extracts all 25 nature entries, maps repository stat constants to PKCalc stat keys, and writes generated proof artifacts under `build/tracker_export/data_migration/`. The generated artifacts should include source data, a `NATURES_BY_ID` shaped JSON, a `calc.NATURES` shaped JSON, and a JavaScript proof file containing `NATURES_BY_ID_PK` and `NATURES_PK` constants.

Add a Playwright script, `tools/tracker_export/pkcalc_data_migration_playwright.cjs`, that opens the live PKCalc app, audits catalog shapes for species, moves, abilities, items, and natures, reads the generated nature proof artifacts, writes `pkcalc_catalog_mapping_report.json`, and writes `natures_migration_validation_report.json`. It must fail if generated natures do not match live PKCalc field expectations or if the mapping report has missing fields or incompatible values for the chosen category.

Add Makefile variables for the data migration directory and reports. Add `tracker-export-data-migration-shape-audit` and `tracker-export-natures-migration-check` as explicit opt-in targets. These targets must not be part of default `make`.

Update `tools/tracker_export/README.md` and `PATCH_NOTES.md` to document the new commands and generated reports.

## Concrete Steps

Run commands from `/home/bayesartre/dev/pokeemerald-expansion-shared-power`.

After implementation, run:

    make tracker-export-data-migration-shape-audit
    make tracker-export-natures-migration-check
    make tracker-export
    make tracker-export-check
    make tracker-export-coverage-check
    make tracker-export-reference-check
    make tracker-export-live-reference-check
    make tracker-export-overlay-check
    make tracker-export-compat-check
    make tracker-export-bundle-check
    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1

## Validation and Acceptance

The shape audit is accepted when `make tracker-export-data-migration-shape-audit` writes `build/tracker_export/data_migration/pkcalc_catalog_mapping_report.json` with live PKCalc catalog field summaries, repository source field mappings, missing fields, incompatible values, and `chosenMigrationCategory` set to `natures`.

The first catalog proof is accepted when `make tracker-export-natures-migration-check` writes `build/tracker_export/data_migration/natures_migration_validation_report.json`, validates the generated nature proof against repository source and live PKCalc expectations through Playwright, and exits zero.

The full slice is accepted only after the existing tracker checks and Docker ROM build listed above pass or any failure is recorded here with reproducible evidence.

## Idempotence and Recovery

All generated data-migration artifacts are under `build/tracker_export/data_migration/` and can be regenerated safely. The Playwright Docker command installs Node dependencies in a temporary container directory and does not write `node_modules` into the repository.

If the live PKCalc app is temporarily unavailable, rerun the Playwright-backed targets when network access recovers. If PKCalc changes its catalog field names, the mapping report should fail with explicit missing fields so a maintainer can update the mapping intentionally.

## Artifacts and Notes

Final verification evidence:

    python3 -m py_compile tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/validate_pkcalc_data_migration.py tools/tracker_export/export_pkcalc_bundle.py tools/tracker_export/validate_pkcalc_bundle.py

    node --check tools/tracker_export/pkcalc_data_migration_playwright.cjs
    node --check tools/tracker_export/live_reference_tracker_export_playwright.cjs

    git diff --check -- Makefile PATCH_NOTES.md tools/tracker_export/README.md tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/pkcalc_data_migration_playwright.cjs tools/tracker_export/validate_pkcalc_data_migration.py .agent/tracker_export_pkcalc_data_migration_execplan.md

    make tracker-export-data-migration-shape-audit
    PKCalc data migration audit passed: chose natures, 25 source entries, 25 NATURES_BY_ID entries

    make tracker-export-natures-migration-check
    PKCalc natures migration proof passed: 25 source natures, 25 by-id entries, 25 calc entries

    make tracker-export
    make tracker-export-check
    Validated build/tracker_export/tracker_data.json

    make tracker-export-coverage-check
    Coverage audit passed for 855 trainers, 854 parties, 1838 sets, and 1975 encounter slots

    make tracker-export-reference-check
    Reference audit passed for generated tracker data

    make tracker-export-live-reference-check
    Live PKCalc reference audit passed for generated tracker data

    make tracker-export-overlay-check
    make tracker-export-bundle-check
    make tracker-export-compat-check

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    EWRAM: 230196 B / 256 KB; IWRAM: 28709 B / 32 KB; ROM: 24709796 B / 32 MB

## Interfaces and Dependencies

New generated paths:

    build/tracker_export/data_migration/source_natures.json
    build/tracker_export/data_migration/pkcalc_natures_by_id.json
    build/tracker_export/data_migration/pkcalc_calc_natures.json
    build/tracker_export/data_migration/pkcalc_natures.js
    build/tracker_export/data_migration/pkcalc_catalog_mapping_report.json
    build/tracker_export/data_migration/natures_migration_validation_report.json

New explicit targets:

    make tracker-export-data-migration-shape-audit
    make tracker-export-natures-migration-check

Revision Note (2026-06-07): Initial plan for the first broad PKCalc data-migration proof slice.
