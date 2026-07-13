# Add PKCalc Compatibility Drift Guardrails

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `.agent/PLANS.md` from the repository root. It is self-contained so a future maintainer can understand and resume the work from only this file and the current tree.

## Purpose / Big Picture

This repository can already generate PKCalc-shaped tracker data and route it into the live PKCalc web app during a Playwright smoke test. The weakness is failure quality: if PKCalc changes an expected data path, global constant name, app helper function, or generation-four data route, the current check can fail as a timeout or a vague browser error. After this change, a maintainer can run an explicit tracker compatibility check and see which PKCalc contract assumption failed.

The observable outcome is a successful `make tracker-export-site-smoke` or `make tracker-export-compat-check` run whose JSON output lists the checked request paths, constants, app globals, and data routes. If PKCalc drifts later, the same command should report named failures such as `requestPaths.sets`, `constants.SETDEX_PK`, `appGlobals.getSetOptions`, or `dataRoutes.setdexGeneration`.

## Progress

- [x] (2026-06-06 22:25Z) Inspected the current tracker export Makefile targets, live site smoke script, tracker README, patch notes, and previous tracker ExecPlans.
- [x] (2026-06-06 22:25Z) Added `tools/tracker_export/pkcalc_compat_contract.json` to name the PKCalc assumptions this repo depends on.
- [x] (2026-06-06 22:25Z) Updated `tools/tracker_export/site_smoke_tracker_export_playwright.cjs` to read the contract, route configured request paths, and report explicit contract checks.
- [x] (2026-06-06 22:25Z) Updated `Makefile` so `tracker-export-site-smoke` passes the contract and added `tracker-export-compat-check` as a named drift-check entry point.
- [x] (2026-06-06 22:27Z) Updated `tools/tracker_export/README.md` with the compatibility contract and drift diagnostics.
- [x] (2026-06-06 22:27Z) Updated `PATCH_NOTES.md` for the tooling, docs, and planning changes.
- [x] (2026-06-06 22:27Z) Ran syntax, tracker export, tracker validation, overlay validation, live site smoke, and named compatibility checks.
- [x] (2026-06-06 22:27Z) Ran the Docker builder command required because the Makefile changed.

## Surprises & Discoveries

- Observation: The existing site smoke used a single broad `page.waitForFunction` that checked `getSetOptions`, `loadDexEntry`, `SETDEX_PK`, `PARTY_ORDER_PK`, and `LOCATIONS`.
  Evidence: `tools/tracker_export/site_smoke_tracker_export_playwright.cjs` waited on those symbols directly, so a missing symbol from upstream PKCalc could surface only as a generic timeout.
- Observation: The overlay target already mirrors PKCalc's live request paths under `build/tracker_export/pkcalc_overlay/js/data/...`.
  Evidence: `tools/tracker_export/README.md` and `Makefile` document and build `js/data/party_order.js`, `js/data/sets.js`, and `js/data/dex/locations.js`.

## Decision Log

- Decision: Store the compatibility assumptions as committed JSON in `tools/tracker_export/pkcalc_compat_contract.json`.
  Rationale: JSON is easy to inspect, diff, and pass into the existing Node Playwright smoke without adding another parser dependency. The file names the external PKCalc contract while keeping fetched or generated app artifacts out of the repository.
  Date/Author: 2026-06-06 / Codex.
- Decision: Put the drift diagnostics inside the live site smoke instead of adding a separate browser script.
  Rationale: The live app is the only place where request-path drift and global-symbol drift can be observed together. Reusing the existing script keeps generated data, overlay layout, and app compatibility in one browser-level proof.
  Date/Author: 2026-06-06 / Codex.
- Decision: Add `tracker-export-compat-check` as a phony alias for the contract-aware site smoke.
  Rationale: The existing `tracker-export-site-smoke` remains the main command, while the named compatibility target gives maintainers and CI logs an explicit drift-check path without wiring it into default `make`.
  Date/Author: 2026-06-06 / Codex.

## Outcomes & Retrospective

Implemented PKCalc compatibility drift guardrails for the tracker export smoke path. Maintainers now have a committed contract file naming PKCalc request paths, generated constants, app globals, `SETDEX[4]`, and `partyOrder`, plus JSON diagnostics from the live site smoke that identify failed assumptions directly. The required tracker checks, live Playwright checks, named compatibility target, and Docker ROM build all passed.

## Context and Orientation

The tracker exporter lives under `tools/tracker_export/`. `export_tracker_data.py` reads source data from trainers, wild encounters, map JSON, and region-map sections, then writes generated files under `build/tracker_export/`. Generated files under `build/` are ignored and should not be committed.

The PKCalc adapter data appears in two layouts. The internal generated layout is `build/tracker_export/pkcalc/sets.js`, `build/tracker_export/pkcalc/party_order.js`, and `build/tracker_export/pkcalc/locations.js`. The overlay layout is `build/tracker_export/pkcalc_overlay/js/data/sets.js`, `build/tracker_export/pkcalc_overlay/js/data/party_order.js`, and `build/tracker_export/pkcalc_overlay/js/data/dex/locations.js`.

PKCalc compatibility means more than the generated files existing. The live app must request the expected data paths, load generated constants named `SETDEX_PK`, `PARTY_ORDER_PK`, and `LOCATIONS`, expose helper functions such as `getSetOptions` and `loadDexEntry`, and wire generation four so `SETDEX[4]` points at `SETDEX_PK` while `partyOrder` points at `PARTY_ORDER_PK`.

## Plan of Work

Add a committed contract file at `tools/tracker_export/pkcalc_compat_contract.json`. The contract must name the live PKCalc request paths, expected generated constants, app globals, generation-four data route, and a small smoke scenario: Sawyer's Geodude and Route 101.

Update `tools/tracker_export/site_smoke_tracker_export_playwright.cjs` so it accepts `--contract`, loads and validates that JSON, derives Playwright request routing from `requestPaths`, and polls for contract readiness. Replace the broad timeout wait with a preflight report that records request path interception counts, symbol types, and named failures. Keep the existing Sawyer and Route 101 smoke checks, but report them under explicit `dataRoutes` and `uiSmoke` contract sections.

Update `Makefile` by adding `PKCALC_COMPAT_CONTRACT`, passing it to `tracker-export-site-smoke`, and adding a phony `tracker-export-compat-check` target that depends on the same live smoke path. Do not add these checks to default `make`.

Update `tools/tracker_export/README.md` with the new contract file, failure diagnostics, and compatibility target. Update `PATCH_NOTES.md` at the top for every changed source or documentation area.

## Concrete Steps

From `/home/bayesartre/dev/pokeemerald-expansion-shared-power`, run:

    node --check tools/tracker_export/site_smoke_tracker_export_playwright.cjs
    make tracker-export-check
    make tracker-export-overlay-check
    make tracker-export-site-smoke
    make tracker-export-compat-check
    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1

The site smoke JSON should include `contractChecks.requestPaths`, `contractChecks.constants`, `contractChecks.appGlobals`, `contractChecks.dataRoutes`, and `failedAssumptions: []`.

## Validation and Acceptance

Acceptance requires all targeted tracker checks to pass. `make tracker-export-check` must regenerate and validate `build/tracker_export/tracker_data.json`. `make tracker-export-overlay-check` must validate the overlay under `build/tracker_export/pkcalc_overlay/`. `make tracker-export-site-smoke` must open the live PKCalc app, route the generated overlay files, and print a successful contract report. `make tracker-export-compat-check` must provide the named drift-check entry point and also pass.

Because `Makefile` changes are part of this work, acceptance also requires the Docker builder command with `NO_MULTIBOOT=1` to complete successfully. Generated or temporary files must remain under ignored `build/` paths or container-local temporary directories.

## Idempotence and Recovery

The tracker export and overlay commands are safe to rerun. They write under `build/tracker_export/` and can be retried after deleting that directory if generated output is stale. The Playwright Docker commands install npm dependencies only in `/tmp/pwprobe` inside the container. If the live PKCalc site changes, the compatibility smoke should fail with named `failedAssumptions`; update `tools/tracker_export/pkcalc_compat_contract.json` only after verifying that the new upstream contract is real and this repository's generated data can still satisfy it.

## Artifacts and Notes

Initial syntax validation succeeded:

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

Overlay validation succeeded:

    make tracker-export-overlay-check
    Validated build/tracker_export/pkcalc_overlay/manifest.json
    Overlay checks passed: file paths, constants, manifest, counts, sources

Live PKCalc site smoke succeeded and reported the checked contract fields:

    make tracker-export-site-smoke
    "status": "ok"
    "failedAssumptions": []
    "contractChecks": {
      "requestPaths": {
        "party": { "path": "/js/data/party_order.js", "passed": true },
        "sets": { "path": "/js/data/sets.js", "passed": true },
        "locations": { "path": "/js/data/dex/locations.js", "passed": true }
      },
      "constants": {
        "SETDEX_PK": { "passed": true },
        "PARTY_ORDER_PK": { "passed": true },
        "LOCATIONS": { "passed": true }
      },
      "appGlobals": {
        "$": { "passed": true },
        "getSetOptions": { "passed": true },
        "loadDexEntry": { "passed": true },
        "SETDEX": { "passed": true }
      },
      "dataRoutes": {
        "setdexGeneration": { "expression": "SETDEX[4] === SETDEX_PK", "passed": true },
        "partyOrder": { "expression": "partyOrder === PARTY_ORDER_PK", "passed": true }
      }
    }

The named compatibility target also succeeded:

    make tracker-export-compat-check
    "status": "ok"
    "failedAssumptions": []

The Docker ROM build required by the Makefile change succeeded:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    arm-none-eabi-objcopy -O binary pokeemerald.elf pokeemerald.gba
    tools/gbafix/gbafix pokeemerald.gba -p --silent

## Interfaces and Dependencies

`tools/tracker_export/site_smoke_tracker_export_playwright.cjs` depends on Node and Playwright. The Makefile invokes it through the official Playwright Docker image configured by `PLAYWRIGHT_IMAGE` and installs the matching `PLAYWRIGHT_PACKAGE` in a temporary directory. The script accepts `--output-dir`, `--adapter-root`, `--contract`, and `--url`.

The compatibility contract schema version is `1`. It contains `requestPaths` for `party`, `sets`, and `locations`; `constants` for `SETDEX_PK`, `PARTY_ORDER_PK`, and `LOCATIONS`; `appGlobals` for `$`, `getSetOptions`, `loadDexEntry`, and `SETDEX`; `dataRoutes` for `SETDEX[4]` and `partyOrder`; and `smokeInputs` for Sawyer's Geodude and Route 101.

Revision Note (2026-06-06): Initial ExecPlan for PKCalc compatibility drift guardrails; captures the implemented approach and remaining validation requirements.

Revision Note (2026-06-06): Recorded final validation evidence and outcome after the tracker export, overlay, live site smoke, named compatibility check, and Docker ROM build passed.
