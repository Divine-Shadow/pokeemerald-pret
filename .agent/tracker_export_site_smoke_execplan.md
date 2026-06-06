# Add real PKCalc compatibility smoke verification

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan is maintained according to `.agent/PLANS.md` from the repository root.

## Purpose / Big Picture

After this change, a maintainer can prove that this repository's generated tracker data is accepted by PKCalc's actual app-style data paths, not only by a local minimal harness. The smoke check opens the PKCalc web app in Chromium, intercepts the app's requests for its trainer set, party order, and location data files, and serves the generated artifacts from `build/tracker_export/` instead.

The result is an explicit `make tracker-export-site-smoke` command. It remains outside default `make` and does not turn this repository into a hosted PKCalc UI. Its purpose is compatibility evidence for the static tracker export only.

## Progress

- [x] (2026-06-06 21:29Z) Inspected the current worktree, existing tracker Makefile targets, and existing tracker smoke script.
- [x] (2026-06-06 21:29Z) Inspected the live PKCalc app and current PKCalc source repository for script order and data consumption paths.
- [x] (2026-06-06 21:29Z) Prototyped Playwright request interception against `https://pkcalc.anastarawneh.com/` with this repo's generated tracker files.
- [x] (2026-06-06 21:29Z) Added `tracker-export-site-smoke` and a committed Playwright site-smoke script.
- [x] (2026-06-06 21:37Z) Ran existing tracker export/check targets.
- [x] (2026-06-06 21:37Z) Ran the existing generated-file Playwright smoke.
- [x] (2026-06-06 21:37Z) Ran the new live PKCalc site compatibility smoke.
- [x] (2026-06-06 21:37Z) Ran `make NO_MULTIBOOT=1` because the Makefile is touched.
- [x] (2026-06-06 21:37Z) Recorded validation evidence and final outcome.

## Surprises & Discoveries

- Observation: PKCalc's app declares data constants with top-level `const`, not as properties on `window`.
  Evidence: The generated data files begin with declarations such as `const SETDEX_PK = ...`; Playwright diagnostics showed `typeof window.SETDEX_PK` was `undefined`, while later scripts and direct page evaluation can access `SETDEX_PK` lexically.

- Observation: The live PKCalc app can be tested without copying its source into this repository.
  Evidence: Playwright request routing intercepted `js/data/party_order.js`, `js/data/sets.js`, and `js/data/dex/locations.js` from the live app and fulfilled them from `build/tracker_export/pkcalc/`.

- Observation: PKCalc's real set and location paths are compact enough to smoke directly.
  Evidence: `src/js/shared_controls.js` uses `SETDEX[4] = SETDEX_PK`, `partyOrder = PARTY_ORDER_PK`, and `getSetOptions()`. `src/js/dex-controls.js` exposes `loadDexEntry("location/route101")`, which renders `LOCATIONS.route101` into `.dex-info`.

## Decision Log

- Decision: Use a live-app request-interception smoke instead of committing a local PKCalc fixture.
  Rationale: This exercises PKCalc's current script order and app functions without vendoring third-party code. It also keeps fetched assets and generated artifacts out of git.
  Date/Author: 2026-06-06 / Codex

- Decision: Add `tracker-export-site-smoke` as a separate target instead of expanding `tracker-export-smoke`.
  Rationale: The existing smoke proves generated constants can load in a minimal page. The new target depends on the live PKCalc app and network access, so it should be explicit and separately diagnosable.
  Date/Author: 2026-06-06 / Codex

- Decision: Let `PKCALC_URL` override the live app URL.
  Rationale: Maintainers may want to test against a local PKCalc build, a staging URL, or the current production site without editing the Makefile.
  Date/Author: 2026-06-06 / Codex

## Outcomes & Retrospective

Implemented the real PKCalc compatibility smoke slice. Maintainers now have `make tracker-export-site-smoke`, which opens the live PKCalc app, routes PKCalc's trainer set, party order, and location data requests to this repository's generated artifacts, and checks PKCalc's own data paths for Sawyer's Geodude and Route 101.

Validation passed for `make tracker-export`, `make tracker-export-check`, the existing generated-file Playwright smoke, the new live PKCalc site smoke, and the Docker `NO_MULTIBOOT=1` ROM build. No third-party PKCalc source was committed.

## Context and Orientation

The tracker exporter writes generated files under `build/tracker_export/`. The PKCalc-shaped adapter files are `build/tracker_export/pkcalc/party_order.js`, `build/tracker_export/pkcalc/sets.js`, and `build/tracker_export/pkcalc/locations.js`. They define `PARTY_ORDER_PK`, `SETDEX_PK`, and `LOCATIONS`.

The live PKCalc app at `https://pkcalc.anastarawneh.com/` loads its own files at `js/data/party_order.js`, `js/data/sets.js`, and `js/data/dex/locations.js`. The site smoke opens that app in Chromium, intercepts those three network requests, and fulfills them from this repository's generated adapter files. All other app scripts and assets still come from the app URL.

PKCalc's own `getSetOptions()` function reads `SETDEX_PK` through `SETDEX[4]` and uses `PARTY_ORDER_PK` to sort trainer set options. PKCalc's own `loadDexEntry("location/route101")` function reads `LOCATIONS.route101` and renders the encounter rows in the Pokédex location view.

## Plan of Work

Add `tools/tracker_export/site_smoke_tracker_export_playwright.cjs`. It accepts `--output-dir` and `--url`, verifies the generated adapter files exist, opens the PKCalc app in Chromium, routes the three data-file requests to local generated artifacts, waits for PKCalc's app functions to load, and evaluates compatibility checks inside the real page.

Add `tracker-export-site-smoke` to the Makefile as a phony explicit utility target. It should depend on `tracker-export-check`, run inside the existing Playwright Docker image, install the matching npm package in a temporary container directory, and run the site-smoke script. It must not become part of default `make`.

Update `tools/tracker_export/README.md` with the new command and `PKCALC_URL` override. Update `PATCH_NOTES.md` for tooling, docs, and planning changes.

## Concrete Steps

Work from the repository root:

    cd /home/bayesartre/dev/pokeemerald-expansion-shared-power

Generate and validate tracker data:

    make tracker-export-check

Run the minimal generated-file browser smoke:

    make tracker-export-smoke

Run the real PKCalc compatibility smoke:

    make tracker-export-site-smoke

Run the ROM build because the Makefile is touched:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1

## Validation and Acceptance

Acceptance requires all of the following evidence. `make tracker-export-check` must regenerate and validate the tracker artifacts, including Sawyer and Route 101 spot checks.

`make tracker-export-smoke` must still pass with `"status": "ok"` for the minimal generated-file browser harness.

`make tracker-export-site-smoke` must pass with `"status": "ok"`. Its output must show that the live app requested and received all three generated adapter files, no page errors occurred, no same-origin critical requests failed, and PKCalc's own app paths passed checks for `pkcalcSetdexPath`, `pkcalcPartyOrderPath`, `pkcalcGetSetOptionsSawyer`, `pkcalcLocationsPath`, and `pkcalcLoadDexEntryRoute101`.

The Docker builder command for `make -j"$(nproc)" NO_MULTIBOOT=1` must exit successfully. Generated JSON/JS files remain under ignored `build/tracker_export/`, and no substantial PKCalc source is committed.

## Idempotence and Recovery

The site smoke is safe to rerun. It writes no source files and reads only generated tracker artifacts. If it fails because the live site is unavailable, rerun with `PKCALC_URL` pointing at a reachable PKCalc build. If generated data is stale, rerun `make tracker-export-check`.

## Artifacts and Notes

Tracker export:

    make tracker-export
    Wrote build/tracker_export/tracker_data.json
    Wrote build/tracker_export/pkcalc/sets.js
    Wrote build/tracker_export/pkcalc/party_order.js
    Wrote build/tracker_export/pkcalc/locations.js

Tracker validation:

    make tracker-export-check
    Validated build/tracker_export/tracker_data.json
    Spot checks passed: TRAINER_SAWYER_1, Route 101 encounters, Route 101 coords

Generated-file browser smoke:

    make tracker-export-smoke
    {
      "status": "ok",
      "checks": {
        "setdexLoaded": true,
        "partyOrderLoaded": true,
        "locationsLoaded": true,
        "sawyerGeodude": true,
        "sawyerParty": true,
        "route101Coord": true,
        "route101Grass": true
      }
    }

Live PKCalc site compatibility smoke:

    make tracker-export-site-smoke
    {
      "status": "ok",
      "appUrl": "https://pkcalc.anastarawneh.com/",
      "intercepted": {
        "party": 1,
        "sets": 1,
        "locations": 1
      },
      "pageErrors": [],
      "consoleErrors": [],
      "failedRequests": [],
      "failedChecks": [],
      "missedInterceptions": [],
      "checks": {
        "pkcalcSetdexPath": true,
        "pkcalcPartyOrderPath": true,
        "pkcalcGetSetOptionsSawyer": true,
        "pkcalcLocationsPath": true,
        "pkcalcLoadDexEntryRoute101": true
      },
      "counts": {
        "setOptions": 2881,
        "renderedRows": 12
      },
      "locationName": "Route 101"
    }

Docker ROM build:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    arm-none-eabi-objcopy -O binary pokeemerald.elf pokeemerald.gba
    tools/gbafix/gbafix pokeemerald.gba -p --silent

## Interfaces and Dependencies

The new target is:

    tracker-export-site-smoke

The new smoke script is:

    tools/tracker_export/site_smoke_tracker_export_playwright.cjs

The target uses these existing Makefile variables:

    PLAYWRIGHT_IMAGE
    PLAYWRIGHT_PACKAGE
    TRACKER_EXPORT_DIR
    TRACKER_EXPORT_TOOL_DIR

The new override variable is:

    PKCALC_URL

Revision Note (2026-06-06): Initial ExecPlan for real PKCalc app compatibility smoke verification through Playwright request interception.

Revision Note (2026-06-06): Recorded completed validation evidence after tracker export/check, generated-file smoke, live PKCalc site smoke, and Docker `NO_MULTIBOOT=1` build passed.
