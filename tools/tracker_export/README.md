# Tracker Export MVP

This directory contains a deterministic static-data exporter for PKCalc-style tracker data.

Run from the repository root:

    make tracker-export
    make tracker-export-check
    make tracker-export-coverage-check
    make tracker-export-reference-check
    make tracker-export-live-reference-check
    make tracker-export-data-migration-shape-audit
    make tracker-export-natures-migration-check
    make tracker-export-identity-migration-check
    make tracker-export-move-metadata-migration-check
    make tracker-export-overlay
    make tracker-export-overlay-check
    make tracker-export-smoke
    make tracker-export-site-smoke
    make tracker-export-compat-check
    make tracker-export-bundle
    make tracker-export-bundle-check

The equivalent direct Python commands are:

    python3 tools/tracker_export/export_tracker_data.py --output-dir build/tracker_export
    python3 tools/tracker_export/validate_tracker_export.py --output-dir build/tracker_export
    python3 tools/tracker_export/audit_tracker_export_coverage.py --output-dir build/tracker_export --report build/tracker_export/coverage_report.json
    python3 tools/tracker_export/audit_tracker_export_references.py --output-dir build/tracker_export --report build/tracker_export/reference_report.json
    python3 tools/tracker_export/export_pkcalc_data_migration.py --output-dir build/tracker_export/data_migration

The exporter reads source data from:

- `src/data/trainers.party`
- `src/data/wild_encounters.json`
- `data/maps/*/map.json`
- `src/data/region_map/region_map_sections.json`

It writes:

- `tracker_data.json`
- `pkcalc/sets.js` containing `const SETDEX_PK = ...;`
- `pkcalc/party_order.js` containing `const PARTY_ORDER_PK = ...;`
- `pkcalc/locations.js` containing `const LOCATIONS = ...;`
- `pkcalc_overlay/` containing PKCalc path-matching data files plus `manifest.json` and `README.txt`
- `coverage_report.json` when `make tracker-export-coverage-check` runs
- `reference_report.json` when `make tracker-export-reference-check` runs
- `live_reference_report.json` when `make tracker-export-live-reference-check` runs
- `data_migration/` reports and proof artifacts when the data-migration targets run
- `pkcalc_tracker_release_bundle.tar.gz` when `make tracker-export-bundle` runs

The maintained compatibility contract is `tools/tracker_export/pkcalc_compat_contract.json`. It names the live PKCalc request paths, generated constants, app globals/functions, and generation-four data route this repository relies on.

The generated JavaScript files are build artifacts. This repository ignores `*.js`, so the Python exporter and this README are the maintained source files.

`make tracker-export-smoke` uses the official Playwright Docker image to load the generated PKCalc adapter files in Chromium. It installs the matching `playwright` npm package in a temporary directory inside the container, then runs `tools/tracker_export/smoke_tracker_export_playwright.cjs`.

`make tracker-export-coverage-check` writes `build/tracker_export/coverage_report.json` and fails if source trainer parties or map-backed wild encounter slots are missing from the generated PKCalc data. Its report includes trainer counts, party-order counts, set counts, map/location counts, skipped records with explicit reasons, and a `failures` list. Current intentional skips are the empty `TRAINER_NONE` party and non-map Battle Pyramid/Battle Pike wild encounter groups.

`make tracker-export-reference-check` writes `build/tracker_export/reference_report.json` and fails if generated tracker references are empty, malformed, missing from source-derived expectations, unexpectedly generated, or normalized into conflicting names. It reports species, moves, abilities, items, natures, types, and encounter species. The current export has no type references.

`make tracker-export-live-reference-check` opens the live PKCalc app in Chromium, intercepts the request paths listed in `pkcalc_compat_contract.json`, and serves this repo's generated overlay. It writes `build/tracker_export/live_reference_report.json` and fails if PKCalc's live Dex catalogs cannot resolve any generated species, move, ability, item, nature, or encounter species reference. The report documents its exhaustive policy, selected live catalog for each category, fallback catalog diagnostics, and an exhaustive `loadDexEntry('location/<id>')` render audit for every generated location with encounters.

`make tracker-export-data-migration-shape-audit` generates broad PKCalc data-migration proof artifacts under `build/tracker_export/data_migration/`, opens the live PKCalc app in Chromium, and writes `pkcalc_catalog_mapping_report.json`, `ability_identity_gap_report.json`, `held_item_identity_gap_report.json`, and `move_metadata_gap_report.json`. The mapping report records live catalog field shapes for species, moves, abilities, items, and natures; maps repo nature source fields to PKCalc fields; records missing fields and incompatible values; and keeps `natures` as the replacement-catalog proof category. The ability and held-item reports are identity-only: they compare generated `{kind, id, name}` catalogs against live `ABILITIES_BY_ID`, `ITEMS_BY_ID`, `calc.ABILITIES[4]`, and `calc.ITEMS[4]`, then list repo-only and live-only gaps without asserting behavior or damage-calculator parity. The move report is identity/basic-metadata only: it compares generated `{kind, id, name, type, category, basePower}` and generated `calc.MOVES[4]`-style `{bp, type, category}` entries against live `MOVES_BY_ID` and `calc.MOVES[4]`, then reports missing fields, repo-only moves, live-only moves, and incompatible shared values without asserting effects, flags, targeting, recoil, multi-hit behavior, or damage-calculator parity.

`make tracker-export-natures-migration-check` reruns the shape audit, then validates `source_natures.json`, `pkcalc_natures_by_id.json`, `pkcalc_calc_natures.json`, `pkcalc_natures.js`, `pkcalc_catalog_mapping_report.json`, and `natures_migration_validation_report.json`. The proof is intentionally not copied into `pkcalc_overlay/` or bundled as a default data replacement.

`make tracker-export-identity-migration-check` reruns the shape audit, then validates the natures proof plus `source_abilities.json`, `pkcalc_abilities_by_id.json`, `pkcalc_abilities.js`, `ability_identity_gap_report.json`, `source_held_items.json`, `pkcalc_held_items_by_id.json`, `pkcalc_held_items.js`, and `held_item_identity_gap_report.json`. Expected live-only and repo-only gaps are allowed when shared ids have compatible identity fields and both gap reports have no `failures`, `missingFields`, or `incompatibleValues`.

`make tracker-export-move-metadata-migration-check` reruns the shape audit, then validates the natures proof plus `source_moves.json`, `pkcalc_moves_by_id.json`, `pkcalc_calc_moves.json`, `pkcalc_moves.js`, and `move_metadata_gap_report.json`. Expected live-only, repo-only, missing-field, and incompatible-value gaps are allowed because this is not a replacement-catalog proof; validation requires structurally complete generated move metadata and an explicit live gap report with no structural `failures`.

`make tracker-export-site-smoke` opens the live PKCalc app in Chromium and intercepts the request paths listed in `pkcalc_compat_contract.json`, serving this repo's generated files instead. It then checks PKCalc's own set-option and location-rendering paths for Sawyer's Geodude and Route 101. Override the app URL with `PKCALC_URL=...` when needed.

`make tracker-export-compat-check` is the named drift-check entry point. It uses the same live site smoke, but its output is intended for diagnosing PKCalc compatibility drift: successful JSON includes `contractChecks.requestPaths`, `contractChecks.constants`, `contractChecks.appGlobals`, and `contractChecks.dataRoutes`; failures include `failedAssumptions` entries such as `requestPaths.sets`, `constants.SETDEX_PK`, `appGlobals.getSetOptions`, or `dataRoutes.setdexGeneration`.

`make tracker-export-bundle` creates `build/tracker_export/pkcalc_tracker_release_bundle.tar.gz`. The archive contains `pkcalc_overlay/`, `coverage_report.json`, `reference_report.json`, `live_reference_report.json`, a `pkcalc_compat_contract.json` snapshot, `bundle_manifest.json` with SHA-256 checksums, and `VERIFY.txt` with the verification commands and overlay paths.

`make tracker-export-bundle-check` unpacks the archive under `build/tracker_export/bundle_check/` and verifies overlay paths, overlay manifest counts, coverage, static reference, and live reference report failures, compatibility contract schema, checksum manifest entries, and generated verification notes.

The overlay directory is a handoff artifact. Copy the contents of `build/tracker_export/pkcalc_overlay/` over a PKCalc build root to replace only:

- `js/data/party_order.js`
- `js/data/sets.js`
- `js/data/dex/locations.js`

`manifest.json` records the source commit, dirty-worktree flag, source files, generated paths, and record counts.

## Scope

This MVP covers static tracker data only: trainer sets, trainer party order, location grid coordinates, and wild encounter tables. The data-migration proof covers natures as the only replacement-catalog proof, plus ability and held-item identity gap reports and a move identity/basic-metadata gap report. None of those proof artifacts are routed into the overlay by default. The project still intentionally does not export the full species database or a full move, item, ability, or damage-calculator database, and it does not implement Lua/save sync or emulator integration.
