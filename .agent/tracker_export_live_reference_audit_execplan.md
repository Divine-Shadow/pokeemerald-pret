# Add a live PKCalc reference-resolution audit

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This document must be maintained in accordance with `.agent/PLANS.md`.

## Purpose / Big Picture

Maintainers already have generated tracker data, a static reference audit, an overlay for PKCalc path compatibility, and a live smoke check that proves one trainer set and one location route still work in the real PKCalc app. The remaining gap is that a generated tracker export can be internally coherent while still containing species, moves, abilities, items, or natures that PKCalc's live app cannot resolve.

After this change, maintainers can run `make tracker-export-live-reference-check` from the repository root. The command opens the live PKCalc app in Chromium through Playwright, intercepts PKCalc's tracker data requests with this repo's generated overlay, compares generated references against PKCalc's live JavaScript environment, writes `build/tracker_export/live_reference_report.json`, and exits nonzero with explicit unresolved names when the live app cannot recognize exported references. The release bundle will include that live report so a reviewer can see the proof artifact alongside coverage and static reference reports.

## Progress

- [x] (2026-06-07 00:13Z) Created this ExecPlan and recorded the milestone scope.
- [x] (2026-06-07 00:20Z) Probed the live PKCalc page with the repo overlay routed in and found safe live Dex catalogs for species, moves, abilities, items, natures, and encounter species ids.
- [x] (2026-06-07 00:21Z) Added `tools/tracker_export/live_reference_tracker_export_playwright.cjs` and the explicit `make tracker-export-live-reference-check` target.
- [x] (2026-06-07 00:24Z) Included `live_reference_report.json` in the PKCalc release bundle inputs, manifest, checksum scope, verification notes, and validator checks.
- [x] (2026-06-07 00:25Z) Updated tracker documentation and `PATCH_NOTES.md` for the live audit and bundle inclusion.
- [x] (2026-06-07 00:27Z) Ran tracker export, validator, coverage audit, static reference audit, live reference audit, overlay check, compatibility check, bundle check, and Docker `make NO_MULTIBOOT=1`; all required checks ended green after rebuilding missing host tools.

## Surprises & Discoveries

- Observation: PKCalc's generation-four damage-calculator catalogs do not contain every exported custom or newer reference, but PKCalc's live Dex identifier catalogs do.
  Evidence: The probe found missing names in `calc.MOVES[4]`, `calc.ABILITIES[4]`, and `calc.ITEMS[4]`, while `MOVES_BY_ID`, `ABILITIES_BY_ID`, `ITEMS_BY_ID`, `SPECIES_BY_ID`, and `NATURES_BY_ID` resolved the generated reference sets.

- Observation: The live location route can be exercised exhaustively for the generated overlay.
  Evidence: The probe found 64 routed locations with encounters and `loadDexEntry('location/<id>')` rendered matching row counts for the first 20 sampled locations.

- Observation: The first Docker ROM build attempt failed because mounted host tools were missing, then the retry passed after rebuilding tools.
  Evidence: The first command printed `tools/scaninc/scaninc: No such file or directory` and `tools/preproc/preproc: No such file or directory`. After running `docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -f make_tools.mk clean-tools tools`, the Docker `make -j"$(nproc)" NO_MULTIBOOT=1` retry exited 0 and printed ROM memory usage.

## Decision Log

- Decision: Keep this as a new explicit check rather than adding it to default `make`.
  Rationale: The audit depends on network access to the live PKCalc site and a Playwright Docker image, so it should be opt-in like the existing site smoke and compatibility check.
  Date/Author: 2026-06-07 / Codex

- Decision: Write generated proof output only under `build/tracker_export/`.
  Rationale: The repository already treats tracker exports and reports as generated artifacts under ignored build paths, while maintained source remains under `tools/tracker_export/`.
  Date/Author: 2026-06-07 / Codex

- Decision: Use live PKCalc Dex identifier catalogs as the primary reference source, with exact-or-normalized matching, and keep damage-calculator generation catalogs only as diagnostics/fallbacks.
  Rationale: The exported ROM data includes newer mechanics and custom species that the old generation-four calculator catalogs may not include, while the live PKCalc Dex catalogs are the app surface used to recognize these names and ids.
  Date/Author: 2026-06-07 / Codex

- Decision: Audit encounter locations exhaustively through `loadDexEntry` for every generated location that has encounters.
  Rationale: The live app exposed a safe route that rendered location encounter rows without persistent browser state, so sampling was unnecessary for the current data size.
  Date/Author: 2026-06-07 / Codex

- Decision: Make `tracker-export-bundle` depend on the live reference check.
  Rationale: The bundle is a proof artifact handoff; including `live_reference_report.json` is only useful if the bundle command regenerates the report before packaging it.
  Date/Author: 2026-06-07 / Codex

## Outcomes & Retrospective

The slice is complete. Maintainers now have `make tracker-export-live-reference-check`, which writes `build/tracker_export/live_reference_report.json` and fails on unresolved live PKCalc references. The report is bundled in `pkcalc_tracker_release_bundle.tar.gz`, covered by bundle validation, and documented in `tools/tracker_export/README.md`.

The final live report resolved species 233/233, moves 266/266, abilities 81/81, items 48/48, natures 17/17, and encounter species 132/132. The exhaustive location route audit rendered 64 locations with encounters and matched 1,975/1,975 rows. The main risk that remains is environmental: the live audit depends on PKCalc's current public app globals and network access, so compatibility drift will surface as a failing explicit check rather than a silent stale bundle.

## Context and Orientation

The tracker exporter lives under `tools/tracker_export/`. `tools/tracker_export/export_tracker_data.py` reads source trainer parties, map metadata, wild encounters, and region map sections, then writes generated tracker data under `build/tracker_export/`. `tools/tracker_export/validate_tracker_export.py` validates the generated JSON shape.

The PKCalc overlay is produced by `tools/tracker_export/export_pkcalc_overlay.py` and checked by `tools/tracker_export/validate_pkcalc_overlay.py`. The overlay is a directory shaped like a PKCalc build root. It contains `js/data/party_order.js`, `js/data/sets.js`, and `js/data/dex/locations.js`, plus metadata files. The existing Makefile targets are `tracker-export-overlay` and `tracker-export-overlay-check`.

`tools/tracker_export/audit_tracker_export_coverage.py` writes `build/tracker_export/coverage_report.json`, proving trainer parties and map-backed wild encounter slots were exported. `tools/tracker_export/audit_tracker_export_references.py` writes `build/tracker_export/reference_report.json`, proving generated species, moves, abilities, items, natures, types, and encounter species match source-derived expectations. That static reference report does not know whether PKCalc's live JavaScript app can resolve those names.

`tools/tracker_export/site_smoke_tracker_export_playwright.cjs` uses the official Playwright Docker image through `make tracker-export-site-smoke`. It opens `https://pkcalc.anastarawneh.com/`, blocks service workers, intercepts the request paths listed in `tools/tracker_export/pkcalc_compat_contract.json`, fulfills them from the generated overlay, and checks PKCalc globals such as `getSetOptions`, `loadDexEntry`, `SETDEX`, and `partyOrder`.

The new live-reference audit should follow the same routing and contract pattern as the site smoke, but it should inspect PKCalc's live reference catalogs or safe app functions for every generated reference category that the live app exposes. A catalog is a JavaScript object or array in the live PKCalc page that contains canonical names for a category such as moves or items. A safe app function is a function already used by PKCalc's page that can be called without mutating persistent browser state or requiring a hosted backend.

The release bundle scripts are `tools/tracker_export/export_pkcalc_bundle.py` and `tools/tracker_export/validate_pkcalc_bundle.py`. They currently include coverage and static reference reports; this slice adds the live report to that bundle and checks that it has no failures.

## Plan of Work

First, generate the overlay if needed and run a one-off Playwright probe against the live PKCalc page. The probe should reuse the compatibility contract's request paths, route `party_order.js`, `sets.js`, and `locations.js` from `build/tracker_export/pkcalc_overlay/`, and print a concise summary of available reference globals or callable resolution helpers. Record any meaningful discovery in this ExecPlan.

Second, add a maintained Node script under `tools/tracker_export/`, tentatively named `live_reference_tracker_export_playwright.cjs`. It should accept `--output-dir`, `--adapter-root`, `--reference-report`, `--report`, `--contract`, and `--url`. It should route the overlay into the live PKCalc app exactly like the site smoke script, wait for the contract preflight, load `reference_report.json`, and compare every exported value in the supported categories against PKCalc's live catalogs or safe app routes. The JSON report should contain the app URL, adapter root, contract path, reference report path, resolution policy, per-category counts, unresolved names, diagnostics, and a top-level `failures` list. If any failure exists, the script exits nonzero after writing the report.

Third, add Makefile variables and a target. `TRACKER_LIVE_REFERENCE_REPORT` should point to `build/tracker_export/live_reference_report.json`. `tracker-export-live-reference-check` should depend on `tracker-export-reference-check` and `tracker-export-overlay-check`, then run the new script inside the same Playwright Docker image pattern used by `tracker-export-site-smoke`.

Fourth, update release bundle generation and validation. `tools/tracker_export/export_pkcalc_bundle.py` should take a `--live-reference-report` argument, require that the report has no failures, copy it into the bundle, include it in `bundle_manifest.json`, and mention the new verification command in `VERIFY.txt`. `tools/tracker_export/validate_pkcalc_bundle.py` should require `live_reference_report.json`, check its schema and failures, include its counts in manifest validation, and require `VERIFY.txt` to mention `make tracker-export-live-reference-check`.

Fifth, update `tools/tracker_export/README.md` so the new command, direct behavior, report path, and bundle inclusion are documented. Update `PATCH_NOTES.md` at the top for each source or documentation change.

## Concrete Steps

Run commands from `/home/bayesartre/dev/pokeemerald-expansion-shared-power`.

Start by ensuring the generated overlay exists and passes local validation:

    make tracker-export-overlay-check

Probe the live page using the Playwright Docker image and the generated overlay. The final maintained implementation may use a new script, but this probe can be an inline Node command because it is diagnostic and writes no maintained source.

After implementing the source changes, run:

    make tracker-export
    make tracker-export-check
    make tracker-export-coverage-check
    make tracker-export-reference-check
    make tracker-export-live-reference-check
    make tracker-export-overlay-check
    make tracker-export-compat-check
    make tracker-export-bundle-check
    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1

If the Docker ROM build fails because host-built tool binaries are incompatible with the container libc, rebuild tools inside the container and retry:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -f make_tools.mk clean-tools tools

## Validation and Acceptance

The new check is accepted when `make tracker-export-live-reference-check` writes `build/tracker_export/live_reference_report.json`, exits zero on the current export, and the report documents whether validation was exhaustive or sampled for each category. The report must include categories for species, moves, abilities, items, natures, and encounter species when PKCalc exposes a safe route for that data. If a category cannot be checked because the live app changed or no safe catalog exists, the report must say that explicitly and fail rather than silently passing.

The bundle change is accepted when `make tracker-export-bundle-check` passes and the unpacked bundle contains `live_reference_report.json`, a manifest entry for that file, and `VERIFY.txt` instructions that include `make tracker-export-live-reference-check`.

The broader slice is accepted when all commands listed in Concrete Steps have been run or a failure is captured here with enough evidence for the next maintainer to reproduce it.

## Idempotence and Recovery

All generated reports and overlays live under `build/tracker_export/` and can be regenerated safely. The Playwright Docker command installs Node dependencies in a temporary container directory and does not write `node_modules` into the repository.

If the live PKCalc site is temporarily unavailable, rerun `make tracker-export-live-reference-check` after network access recovers. If the contract preflight fails because PKCalc changed global names or request paths, update `tools/tracker_export/pkcalc_compat_contract.json` only after verifying the new live app behavior and record the decision in this plan.

If validation reveals a generated reference that PKCalc truly cannot resolve, fix the exporter normalization or source mapping where appropriate. Do not hide the missing name in the live audit unless there is a documented PKCalc incompatibility that must remain a known failure.

## Artifacts and Notes

Initial local overlay and static reference commands passed before implementation:

    make tracker-export-overlay-check
    Overlay checks passed: file paths, constants, manifest, counts, sources

    make tracker-export-reference-check
    Reference categories: species: 233, moves: 266, abilities: 81, items: 48, natures: 17, types: 0, encounterSpecies: 132
    Reference audit passed: no malformed, missing, unexpected, or conflicting references

Final verification evidence:

    node --check tools/tracker_export/live_reference_tracker_export_playwright.cjs
    python3 -m py_compile tools/tracker_export/export_pkcalc_bundle.py tools/tracker_export/validate_pkcalc_bundle.py

    make tracker-export
    Wrote build/tracker_export/tracker_data.json
    Wrote build/tracker_export/pkcalc/sets.js
    Wrote build/tracker_export/pkcalc/party_order.js
    Wrote build/tracker_export/pkcalc/locations.js

    make tracker-export-check
    Validated build/tracker_export/tracker_data.json
    Spot checks passed: TRAINER_SAWYER_1, Route 101 encounters, Route 101 coords

    make tracker-export-coverage-check
    Trainer coverage: 855/855 trainers, 854/854 parties, 1838/1838 sets, 1 skipped empty-party trainer(s)
    Location coverage: 124 map-backed wild header(s), 1975/1975 encounter slots, 2 skipped non-map group(s), 11 skipped unsupported header(s)
    Coverage audit passed: no missing or unexpected tracker records

    make tracker-export-reference-check
    Reference categories: species: 233, moves: 266, abilities: 81, items: 48, natures: 17, types: 0, encounterSpecies: 132
    Reference audit passed: no malformed, missing, unexpected, or conflicting references

    make tracker-export-live-reference-check
    Live reference audit passed: species: 233, moves: 266, abilities: 81, items: 48, natures: 17, encounterSpecies: 132
    Encounter route audit passed: 64 locations, 1975 rows

    make tracker-export-overlay-check
    Overlay checks passed: file paths, constants, manifest, counts, sources

    make tracker-export-bundle-check
    Bundle checks passed: overlay paths, manifest, coverage, references, live references, contract, checksums, VERIFY.txt

    make tracker-export-compat-check
    "status": "ok"
    "pkcalcLoadDexEntryRoute101": true

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    Memory region         Used Size  Region Size  %age Used
               EWRAM:      230196 B       256 KB     87.81%
               IWRAM:       28709 B        32 KB     87.61%
                 ROM:    24709796 B        32 MB     73.64%

## Interfaces and Dependencies

The new script should be a CommonJS Node script like `site_smoke_tracker_export_playwright.cjs` because the Playwright Docker invocation already runs CommonJS scripts with `node`. It should depend only on Node built-ins and the `playwright` package installed in the temporary container directory.

The Makefile should continue using these existing variables:

    TRACKER_EXPORT_DIR := build/tracker_export
    TRACKER_OVERLAY_DIR := build/tracker_export/pkcalc_overlay
    TRACKER_REFERENCE_REPORT := build/tracker_export/reference_report.json
    PKCALC_COMPAT_CONTRACT := tools/tracker_export/pkcalc_compat_contract.json
    PLAYWRIGHT_IMAGE := mcr.microsoft.com/playwright:v1.55.0-noble
    PLAYWRIGHT_PACKAGE := playwright@1.55.0
    PKCALC_URL := https://pkcalc.anastarawneh.com/

The new generated report path should be:

    build/tracker_export/live_reference_report.json

The new Makefile target should be:

    make tracker-export-live-reference-check

Revision Note (2026-06-07): Initial plan for adding live PKCalc reference-resolution auditing and bundle inclusion.
