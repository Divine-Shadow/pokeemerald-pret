# Add PKCalc overlay export artifacts

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan is maintained according to `.agent/PLANS.md` from the repository root.

## Purpose / Big Picture

After this change, a maintainer can run explicit Makefile commands that create a drop-in PKCalc tracker data overlay under `build/tracker_export/pkcalc_overlay/`. The overlay mirrors PKCalc's expected data file paths, so it can be copied over a PKCalc build root to replace only trainer party order, trainer sets, and locations. A manifest makes the artifact auditable by recording source files, generated counts, commit metadata, and the dirty-worktree flag.

The overlay does not add full Dex data, damage-calculator correctness, Lua/save sync, emulator sync, or hosted UI support. It packages the static tracker data that this repository already exports.

## Progress

- [x] (2026-06-06 21:48Z) Inspected the current worktree, uncommitted tracker targets, tracker README, and patch-note ordering.
- [x] (2026-06-06 21:48Z) Added `tracker-export-overlay` and `tracker-export-overlay-check` Makefile targets without wiring them into default `make`.
- [x] (2026-06-06 21:48Z) Added overlay generation and validation scripts under `tools/tracker_export/`.
- [x] (2026-06-06 21:48Z) Updated the live PKCalc site smoke script so it can consume the overlay's PKCalc path layout.
- [x] (2026-06-06 21:48Z) Documented the overlay flow and handoff paths in `tools/tracker_export/README.md`.
- [x] (2026-06-06 22:05Z) Ran existing tracker export/check targets.
- [x] (2026-06-06 22:05Z) Ran overlay export/check targets.
- [x] (2026-06-06 22:05Z) Ran live PKCalc site smoke using the overlay paths.
- [x] (2026-06-06 22:05Z) Ran `make NO_MULTIBOOT=1` because the Makefile is touched.
- [x] (2026-06-06 22:05Z) Recorded validation evidence and final outcome.

## Surprises & Discoveries

- Observation: The live PKCalc site smoke can consume either the older `pkcalc/` generated adapter layout or the new overlay layout.
  Evidence: `tools/tracker_export/site_smoke_tracker_export_playwright.cjs` now accepts `--adapter-root`, which points at a directory containing `js/data/party_order.js`, `js/data/sets.js`, and `js/data/dex/locations.js`.

- Observation: The repository worktree is intentionally dirty from unrelated gameplay, Nix, AGENTS, and documentation work.
  Evidence: `git status --short --untracked-files=all` lists those files alongside tracker changes. The overlay manifest records `repo.dirty: true` when such changes are present, so consumers know the artifact came from a dirty checkout.

## Decision Log

- Decision: Implement overlay creation as a separate adapter script instead of changing the canonical exporter.
  Rationale: `export_tracker_data.py` remains responsible for canonical data and PKCalc constants. The overlay is packaging, so keeping it separate preserves a clean boundary and makes validation simpler.
  Date/Author: 2026-06-06 / Codex

- Decision: Put overlay files under `build/tracker_export/pkcalc_overlay/`.
  Rationale: The directory is ignored build output, and its contents can be copied directly over a PKCalc build root without committing generated JavaScript.
  Date/Author: 2026-06-06 / Codex

- Decision: Include `generatedAt`, repo commit, dirty flag, source files, adapter paths, counts, and post-MVP gaps in `manifest.json`.
  Rationale: Those fields make the generated handoff artifact auditable without expanding the data scope.
  Date/Author: 2026-06-06 / Codex

## Outcomes & Retrospective

Implemented the PKCalc overlay export artifact. Maintainers now have `make tracker-export-overlay` and `make tracker-export-overlay-check`, which create and validate a drop-in `build/tracker_export/pkcalc_overlay/` directory. The live PKCalc site smoke now consumes the overlay path layout through `--adapter-root`.

Validation passed for existing tracker export/check, overlay export/check, live PKCalc site smoke using overlay paths, the existing minimal browser smoke, and the Docker `NO_MULTIBOOT=1` ROM build.

## Context and Orientation

The canonical tracker exporter writes `build/tracker_export/tracker_data.json` plus three generated PKCalc constants under `build/tracker_export/pkcalc/`: `party_order.js`, `sets.js`, and `locations.js`.

PKCalc's app expects those data files at different paths inside a PKCalc build root: `js/data/party_order.js`, `js/data/sets.js`, and `js/data/dex/locations.js`. The overlay target creates that exact path layout under `build/tracker_export/pkcalc_overlay/`.

`manifest.json` is a small JSON file that records where the overlay came from. The dirty-worktree flag means `git status --porcelain` was non-empty at generation time.

## Plan of Work

Add `tools/tracker_export/export_pkcalc_overlay.py`. It reads `build/tracker_export/tracker_data.json` and copies the generated adapter files into the PKCalc path layout under `build/tracker_export/pkcalc_overlay/`. It writes `manifest.json` and `README.txt`.

Add `tools/tracker_export/validate_pkcalc_overlay.py`. It checks the overlay files exist, unwraps the expected JavaScript constants, compares payloads to `tracker_data.json`, validates manifest fields and counts, and verifies the handoff README mentions the expected paths.

Update `Makefile` with `tracker-export-overlay` and `tracker-export-overlay-check`. The overlay export target depends on `tracker-export-check`; the overlay check target depends on overlay export. Update `tracker-export-site-smoke` to depend on `tracker-export-overlay-check` and pass the overlay directory to the site smoke script.

Update `tools/tracker_export/README.md` and `PATCH_NOTES.md`.

## Concrete Steps

Work from the repository root:

    cd /home/bayesartre/dev/pokeemerald-expansion-shared-power

Run the existing tracker export/check:

    make tracker-export
    make tracker-export-check

Generate and validate the overlay:

    make tracker-export-overlay
    make tracker-export-overlay-check

Run the live PKCalc site smoke using the overlay layout:

    make tracker-export-site-smoke

Run the ROM build because the Makefile is touched:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1

## Validation and Acceptance

Acceptance requires `make tracker-export-overlay` to write these files:

    build/tracker_export/pkcalc_overlay/js/data/party_order.js
    build/tracker_export/pkcalc_overlay/js/data/sets.js
    build/tracker_export/pkcalc_overlay/js/data/dex/locations.js
    build/tracker_export/pkcalc_overlay/manifest.json
    build/tracker_export/pkcalc_overlay/README.txt

`make tracker-export-overlay-check` must validate file presence, constant names, payload equality with `tracker_data.json`, manifest schema, source metadata, counts, and README path mentions.

`make tracker-export-site-smoke` must pass with the overlay directory as its adapter root, proving the live PKCalc app can consume the overlay paths through PKCalc's own set-option and location-rendering code.

The Docker builder command for `make -j"$(nproc)" NO_MULTIBOOT=1` must exit successfully. Generated overlay files must remain ignored under `build/`, and no PKCalc source is committed.

## Idempotence and Recovery

The overlay target is safe to rerun. It deletes and recreates only the chosen overlay output directory. If validation fails, rerun `make tracker-export-overlay-check` after fixing the script or deleting `build/tracker_export/pkcalc_overlay/`.

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

Overlay export:

    make tracker-export-overlay
    Wrote build/tracker_export/pkcalc_overlay
    Wrote build/tracker_export/pkcalc_overlay/js/data/party_order.js
    Wrote build/tracker_export/pkcalc_overlay/js/data/sets.js
    Wrote build/tracker_export/pkcalc_overlay/js/data/dex/locations.js
    Wrote build/tracker_export/pkcalc_overlay/manifest.json
    Wrote build/tracker_export/pkcalc_overlay/README.txt

Overlay validation:

    make tracker-export-overlay-check
    Validated build/tracker_export/pkcalc_overlay/manifest.json
    Overlay checks passed: file paths, constants, manifest, counts, sources

Manifest summary:

    schemaVersion 1
    artifact pkcalc-overlay
    commit f6863b54a310
    dirty True
    files {'partyOrder': 'js/data/party_order.js', 'sets': 'js/data/sets.js', 'locations': 'js/data/dex/locations.js'}
    counts {'trainers': 855, 'setdexSpecies': 233, 'partyOrder': 854, 'locations': 213, 'route101Encounters': 12}

Live PKCalc site smoke using overlay paths:

    make tracker-export-site-smoke
    {
      "status": "ok",
      "adapterRoot": "/workspace/build/tracker_export/pkcalc_overlay",
      "intercepted": {
        "party": 1,
        "sets": 1,
        "locations": 1
      },
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

Existing generated-file browser smoke:

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

Docker ROM build:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    arm-none-eabi-objcopy -O binary pokeemerald.elf pokeemerald.gba
    tools/gbafix/gbafix pokeemerald.gba -p --silent

## Interfaces and Dependencies

The new Makefile targets are:

    tracker-export-overlay
    tracker-export-overlay-check

The new scripts are:

    tools/tracker_export/export_pkcalc_overlay.py
    tools/tracker_export/validate_pkcalc_overlay.py

The overlay directory is:

    build/tracker_export/pkcalc_overlay

Revision Note (2026-06-06): Initial ExecPlan for generating and validating a PKCalc path-matching static tracker overlay.

Revision Note (2026-06-06): Recorded completed validation evidence after tracker export/check, overlay export/check, live site smoke with overlay paths, minimal browser smoke, and Docker `NO_MULTIBOOT=1` build passed.
