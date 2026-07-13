# Generate PKCalc ability and held-item identity catalog proofs

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This document must be maintained in accordance with `.agent/PLANS.md`.

## Purpose / Big Picture

The tracker export can already prove a small natures catalog generated from this repository matches PKCalc's live data shape. The next useful migration step is to prove identity catalogs for abilities and held items: lists of PKCalc-compatible `{kind, id, name}` objects generated from repo source data, with explicit reports showing what matches live PKCalc and what remains repo-only or live-only. After this change, maintainers can run opt-in make targets to see exactly how far ability and held-item identity migration can go without claiming ability behavior, item behavior, or damage-calculator correctness.

This slice does not replace PKCalc's live catalogs. It writes proof artifacts under `build/tracker_export/data_migration/` and validates them through the live PKCalc app using Playwright.

## Progress

- [x] (2026-06-07 03:17Z) Created this ExecPlan after inspecting the existing natures migration exporter, Playwright validator, Makefile targets, live-reference catalog flattening, `src/data/abilities.h`, `include/constants/abilities.h`, `src/data/items.h`, `include/constants/items.h`, and `include/constants/hold_effects.h`.
- [x] (2026-06-07 03:25Z) Extended the data-migration exporter so it emits ability identity artifacts and held-item identity artifacts from repository source data.
- [x] (2026-06-07 03:29Z) Extended the Playwright audit so it compares generated ability/item identities against live `ABILITIES_BY_ID`, `ITEMS_BY_ID`, `calc.ABILITIES[4]`, and `calc.ITEMS[4]`, and writes explicit gap reports.
- [x] (2026-06-07 03:31Z) Added the opt-in `tracker-export-identity-migration-check` Makefile validation target, documentation, and patch notes.
- [x] (2026-06-07 03:35Z) Tightened identity reports to include explicit `customRepoOnly` and `incompatibleIds` arrays, then reran the new targets, existing tracker checks, compatibility and bundle checks, and Docker `make NO_MULTIBOOT=1`.

## Surprises & Discoveries

- Observation: Live PKCalc ability and item identifier catalogs are arrays partitioned by generation rather than flat objects.
  Evidence: The existing `build/tracker_export/data_migration/pkcalc_catalog_mapping_report.json` records `ABILITIES_BY_ID` as an array with sample path `3.airlock`, fields `id`, `kind`, and `name`; `ITEMS_BY_ID` as an array with sample path `2.berryjuice`, fields `id`, `kind`, `megaEvolves`, and `name`; `calc.ABILITIES[4]` as a 126-entry string array; and `calc.ITEMS[4]` as a 223-entry string array.

- Observation: Ability source data is complete for identity and intentionally broader than PKCalc's Generation 4 calculator catalog.
  Evidence: `src/data/abilities.h` defines `gAbilitiesInfo[ABILITIES_COUNT]` entries with `.name`, `.description`, `.aiRating`, and behavior flags. `include/constants/abilities.h` assigns numeric `ABILITY_*` constants. This slice should export names and ids, while reporting descriptions and flags as source-only fields outside PKCalc identity shape.

- Observation: Item source data includes all bag items, so the safe identity proof should focus on held items.
  Evidence: `src/data/items.h` defines `gItemsInfo[]` entries for balls, medicine, treasures, key items, berries, and held items. The `struct Item` fields in `include/item.h` include `.holdEffect`, `.holdEffectParam`, `.pocket`, and `.sortType`, and `include/constants/hold_effects.h` defines `HOLD_EFFECT_NONE` and battle hold effects. This slice will include entries that have a non-`HOLD_EFFECT_NONE` `.holdEffect`, plus report any live PKCalc item identities that are not generated from repo held-item data.

- Observation: The live identity audit validates with expected gaps rather than exact catalog equality.
  Evidence: `make tracker-export-identity-migration-check` generated 310 ability identities and 323 held-item identities. The ability gap report had 304 shared live ids, 6 generated ids missing from live `ABILITIES_BY_ID`, 9 live-only ids, 187 generated names missing from `calc.ABILITIES[4]`, and no missing fields or incompatible values. The held-item gap report had 315 shared live ids, 8 generated ids missing from live `ITEMS_BY_ID`, 229 live-only ids, 177 generated names missing from `calc.ITEMS[4]`, and no missing fields or incompatible values.

## Decision Log

- Decision: Export ability and held-item identity proofs, not behavior catalogs.
  Rationale: Live PKCalc identity surfaces require only `kind`, `id`, and `name` for abilities and at least `kind`, `id`, and `name` for items. Ability behavior flags, item hold effects, parameters, mega evolution metadata, and calculator damage behavior need category-specific semantics and are outside this slice.
  Date/Author: 2026-06-07 / Codex

- Decision: Treat all repo abilities except `ABILITY_NONE` as candidates, and treat only items with non-empty names and non-`HOLD_EFFECT_NONE` hold effects as held-item candidates.
  Rationale: `ABILITY_NONE` is a sentinel, not a user-facing calculator ability. The item table contains many bag-only records that are not relevant to held-item damage calculation, so a held-effect filter gives a conservative identity proof while preserving explicit gap reports for live-only and repo-only item names.
  Date/Author: 2026-06-07 / Codex

## Outcomes & Retrospective

Completed. The repository now has opt-in PKCalc ability and held-item identity catalog proofs alongside the existing natures proof. The exporter generates source reports, PKCalc-shaped `{kind, id, name}` identity catalogs, and JavaScript proof files for 310 abilities and 323 held items. The Playwright audit reads live PKCalc ability and item identity surfaces, writes explicit gap reports, and treats repo-only/live-only differences as reported migration gaps rather than behavior failures.

The final reports show no missing fields, no incompatible shared-id values, and no validation failures for the ability or held-item identity proofs. The generated artifacts remain under `build/tracker_export/data_migration/` and are not wired into the overlay or default PKCalc replacement path.

The ability and held-item gap reports both include `missingFromLiveById` and the explicit alias `customRepoOnly` for generated entries not present in live PKCalc, plus `incompatibleIds` for shared ids whose live identity id does not match the generated id. Final validation requires `incompatibleIds` to be empty.

## Context and Orientation

Tracker export tooling lives under `tools/tracker_export/`. The existing data-migration proof has three pieces: `tools/tracker_export/export_pkcalc_data_migration.py` generates source-derived JSON and JavaScript artifacts; `tools/tracker_export/pkcalc_data_migration_playwright.cjs` opens the live PKCalc app and validates generated artifacts against live globals; `tools/tracker_export/validate_pkcalc_data_migration.py` validates the generated reports offline. The Makefile target `tracker-export-data-migration-shape-audit` runs the exporter and Playwright audit, and `tracker-export-natures-migration-check` runs the offline validator.

PKCalc's live ability identity catalog is `ABILITIES_BY_ID`. It is an array whose generation entries contain objects keyed by normalized ids such as `airlock`, with values shaped like `{kind:"Ability", id:"airlock", name:"Air Lock"}`. PKCalc's live item identity catalog is `ITEMS_BY_ID`. It is also generation-partitioned, with item entries keyed by normalized ids such as `berryjuice`, and values containing `kind`, `id`, `name`, and sometimes extra fields such as `megaEvolves`. PKCalc also exposes calculator arrays `calc.ABILITIES[4]` and `calc.ITEMS[4]`, where values are display-name strings for Generation 4.

The repository source of truth for ability identity is `src/data/abilities.h`, in `gAbilitiesInfo[ABILITIES_COUNT]`. Numeric ability constants live in `include/constants/abilities.h`. The repository source of truth for item identity and held-effect status is `src/data/items.h`, in `gItemsInfo[]`. Numeric item constants live in `include/constants/items.h`, and hold-effect constants live in `include/constants/hold_effects.h`.

An "identity catalog" in this plan means a generated map from PKCalc id to `{kind, id, name}` objects. It does not mean a complete damage-calculator catalog. This distinction matters because PKCalc can resolve a name or id without necessarily calculating the same battle behavior as this ROM.

## Plan of Work

Extend `tools/tracker_export/export_pkcalc_data_migration.py` so it still writes the existing natures artifacts and also writes source reports plus PKCalc-shaped identity artifacts for abilities and held items. The ability report should include constant, numeric index, display name, PKCalc id, description, AI rating, and behavior flags for traceability, but the PKCalc identity artifact should contain only `kind`, `id`, and `name`. The held-item report should include constant, numeric index, display name, PKCalc id, `holdEffect`, `holdEffectParam`, `sortType`, `pocket`, and `type` where available, while the PKCalc identity artifact should contain `kind`, `id`, and `name`.

Extend `tools/tracker_export/pkcalc_data_migration_playwright.cjs` so it reads the generated ability/item artifacts, flattens live generation-partitioned PKCalc catalogs, compares ids and display names, and writes explicit `ability_identity_gap_report.json` and `held_item_identity_gap_report.json` files. The gap reports must list generated entries, generated entries missing from live identity catalogs, live entries missing from the generated proof, generated entries missing from `calc.ABILITIES[4]` or `calc.ITEMS[4]`, and any incompatible values where a shared id has a different name or required field. The reports may have status `ok_with_gaps` when gaps are present but the generated artifact itself is valid; they should fail only when source extraction, required field validation, or shared-id value comparison is broken.

Add Makefile variables and opt-in targets for the new reports. The target `tracker-export-identity-migration-check` should run the shape audit and then validate natures, abilities, and held items. Keep these targets out of default `make`.

Update `tools/tracker_export/README.md` to document the new commands and generated files. Update `PATCH_NOTES.md` at the top for each source/doc change.

## Concrete Steps

Run commands from `/home/bayesartre/dev/pokeemerald-expansion-shared-power`.

After implementation, run:

    python3 -m py_compile tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/validate_pkcalc_data_migration.py tools/tracker_export/export_pkcalc_bundle.py tools/tracker_export/validate_pkcalc_bundle.py
    node --check tools/tracker_export/pkcalc_data_migration_playwright.cjs
    git diff --check -- Makefile PATCH_NOTES.md tools/tracker_export/README.md tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/pkcalc_data_migration_playwright.cjs tools/tracker_export/validate_pkcalc_data_migration.py .agent/tracker_export_pkcalc_identity_catalogs_execplan.md
    make tracker-export-data-migration-shape-audit
    make tracker-export-natures-migration-check
    make tracker-export-identity-migration-check
    make tracker-export
    make tracker-export-check
    make tracker-export-coverage-check
    make tracker-export-reference-check
    make tracker-export-live-reference-check
    make tracker-export-overlay-check
    make tracker-export-bundle-check
    make tracker-export-compat-check
    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1

## Validation and Acceptance

The exporter is accepted when it writes ability and held-item source reports, PKCalc-shaped identity JSON, and JavaScript proof files under `build/tracker_export/data_migration/`, and exits zero only when source extraction has no structural failures.

The Playwright audit is accepted when it opens live PKCalc, reads `ABILITIES_BY_ID`, `ITEMS_BY_ID`, `calc.ABILITIES[4]`, and `calc.ITEMS[4]`, writes ability/item gap reports, and fails only for invalid generated artifacts or incompatible shared-id field values. Live-only and repo-only entries are expected gaps and must be reported explicitly.

The full slice is accepted only after the new opt-in migration check, existing tracker checks, compatibility and bundle checks, and Docker ROM build pass or any failure is captured here with reproducible evidence.

## Idempotence and Recovery

All generated data-migration artifacts live under `build/tracker_export/data_migration/` and can be regenerated safely. The Playwright Docker command installs Node dependencies in a temporary container directory and does not write `node_modules` into the repository.

If the live PKCalc app is unavailable, rerun the Playwright-backed targets when network access recovers. If PKCalc changes ability or item field names, the reports should identify missing fields or incompatible shared-id values so maintainers can update mappings intentionally.

If Docker ROM builds fail with `tools/gbafix/gbafix: No such file or directory`, inspect `readelf -l tools/gbafix/gbafix`. A Nix store interpreter means host tools were built with the Nix toolchain and cannot run inside Docker. Rebuild tools inside the builder image with:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -f make_tools.mk clean-tools tools CC=/usr/bin/gcc CXX=/usr/bin/g++ LDFLAGS=

## Artifacts and Notes

Final verification evidence:

    python3 -m py_compile tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/validate_pkcalc_data_migration.py tools/tracker_export/export_pkcalc_bundle.py tools/tracker_export/validate_pkcalc_bundle.py

    node --check tools/tracker_export/pkcalc_data_migration_playwright.cjs
    node --check tools/tracker_export/live_reference_tracker_export_playwright.cjs

    git diff --check -- Makefile PATCH_NOTES.md tools/tracker_export/README.md tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/pkcalc_data_migration_playwright.cjs tools/tracker_export/validate_pkcalc_data_migration.py .agent/tracker_export_pkcalc_identity_catalogs_execplan.md

    make tracker-export-natures-migration-check
    PKCalc natures migration proof passed: 25 source natures, 25 by-id entries, 25 calc entries

    make tracker-export-identity-migration-check
    PKCalc data migration audit passed: chose natures, 25 source entries, 25 NATURES_BY_ID entries; ability identity ok_with_gaps, held-item identity ok_with_gaps
    PKCalc identity migration proof passed: 310 abilities (ok_with_gaps), 323 held items (ok_with_gaps)

    make tracker-export
    make tracker-export-check
    Validated build/tracker_export/tracker_data.json

    make tracker-export-coverage-check
    Trainer coverage: 855/855 trainers, 854/854 parties, 1838/1838 sets
    Location coverage: 1975/1975 encounter slots

    make tracker-export-reference-check
    Reference categories: species: 233, moves: 266, abilities: 81, items: 48, natures: 17, types: 0, encounterSpecies: 132

    make tracker-export-live-reference-check
    Live reference audit passed: species: 233, moves: 266, abilities: 81, items: 48, natures: 17, encounterSpecies: 132
    Encounter route audit passed: 64 locations, 1975 rows

    make tracker-export-overlay-check
    Overlay checks passed: file paths, constants, manifest, counts, sources

    make tracker-export-bundle-check
    Bundle checks passed: overlay paths, manifest, coverage, references, live references, contract, checksums, VERIFY.txt

    make tracker-export-compat-check
    status: ok; intercepted party, sets, and locations once each; Sawyer Geodude and Route 101 UI smoke passed

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    EWRAM: 230200 B / 256 KB; IWRAM: 28709 B / 32 KB; ROM: 24709840 B / 32 MB

Expected new generated paths:

    build/tracker_export/data_migration/source_abilities.json
    build/tracker_export/data_migration/pkcalc_abilities_by_id.json
    build/tracker_export/data_migration/pkcalc_abilities.js
    build/tracker_export/data_migration/ability_identity_gap_report.json
    build/tracker_export/data_migration/source_held_items.json
    build/tracker_export/data_migration/pkcalc_held_items_by_id.json
    build/tracker_export/data_migration/pkcalc_held_items.js
    build/tracker_export/data_migration/held_item_identity_gap_report.json

## Interfaces and Dependencies

The new Python exporter functions should reuse the existing `OrderedDict` and JSON-writing style in `tools/tracker_export/export_pkcalc_data_migration.py`. The JavaScript validator should reuse the existing live-catalog probe style in `tools/tracker_export/pkcalc_data_migration_playwright.cjs` and the recursive catalog name collection approach from `tools/tracker_export/live_reference_tracker_export_playwright.cjs`.

New explicit targets expected by the end of this plan:

    make tracker-export-identity-migration-check

Revision Note (2026-06-07): Initial plan for ability and held-item identity catalog migration proofs.

Revision Note (2026-06-07): Recorded implementation of exporter, live gap reports, Makefile target, documentation, and initial identity validation evidence.

Revision Note (2026-06-07): Recorded final validation evidence and outcome for the PKCalc ability and held-item identity catalog proof slice.

Revision Note (2026-06-07): Documented the explicit `customRepoOnly` and `incompatibleIds` gap-report fields and reran final validation.
