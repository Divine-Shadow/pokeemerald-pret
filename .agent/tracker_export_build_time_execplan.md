# Add build-time tracker export and browser smoke checks

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan is maintained according to `.agent/PLANS.md` from the repository root.

## Purpose / Big Picture

After this change, a maintainer can use Makefile targets instead of remembering direct Python commands to generate and verify PKCalc-style tracker data. The default ROM build remains unchanged. The new explicit flow creates `build/tracker_export/tracker_data.json`, creates the generated JavaScript adapter files for `SETDEX_PK`, `PARTY_ORDER_PK`, and `LOCATIONS`, validates their JSON-backed shape, and runs a Chromium smoke check through Playwright to prove the generated constants work in a browser context.

This builds on the committed static tracker MVP in `tools/tracker_export/`. The new slice does not expand tracker data scope. It only makes the export easier to invoke and gives maintainers a repeatable browser-level check.

## Progress

- [x] (2026-06-06 21:22Z) Inspected the current worktree, Makefile target layout, `.agent/PLANS.md`, `tools/tracker_export/README.md`, and the committed MVP ExecPlan.
- [x] (2026-06-06 21:22Z) Added explicit `tracker-export`, `tracker-export-check`, and `tracker-export-smoke` Makefile targets without wiring them into default `make`.
- [x] (2026-06-06 21:22Z) Added a committed Playwright smoke script under `tools/tracker_export/`.
- [x] (2026-06-06 21:22Z) Documented the Makefile flow and browser smoke command in `tools/tracker_export/README.md`.
- [x] (2026-06-06 21:25Z) Ran tracker export and tracker export check inside `pokeemerald-expansion:builder`.
- [x] (2026-06-06 21:25Z) Ran the Playwright smoke target through Docker.
- [x] (2026-06-06 21:25Z) Ran `make NO_MULTIBOOT=1` because the Makefile is touched.
- [x] (2026-06-06 21:25Z) Recorded validation evidence and final outcome.

## Surprises & Discoveries

- Observation: This repository ignores `*.js` globally.
  Evidence: The prior MVP plan and `.gitignore` show generated PKCalc adapter files must remain in ignored build output. The committed smoke test is therefore named `smoke_tracker_export_playwright.cjs` rather than `*.js`.

- Observation: The Makefile already has a lightweight-target escape hatch.
  Evidence: `RULES_NO_SCAN` prevents selected utility targets from building tools and generating map sources before they run. The tracker targets are added there so `make tracker-export` stays a fast explicit utility command.

- Observation: The Playwright smoke target can run without adding local Node dependencies.
  Evidence: `make tracker-export-smoke` installs `playwright@1.55.0` in `/tmp/pwprobe` inside `mcr.microsoft.com/playwright:v1.55.0-noble`, then runs the committed `.cjs` script through `NODE_PATH=/tmp/pwprobe/node_modules`.

## Decision Log

- Decision: Add `tracker-export`, `tracker-export-check`, and `tracker-export-smoke` as explicit phony Makefile targets.
  Rationale: These names describe the maintainer actions directly, avoid changing the default ROM build, and provide a one-command browser smoke path when Docker is available.
  Date/Author: 2026-06-06 / Codex

- Decision: Have `tracker-export-smoke` use the official Playwright Docker image and install the matching npm package in a temporary container directory.
  Rationale: The local workspace does not have a Playwright package or Chromium binary, while the Docker image provides the browser runtime. Installing the npm package in `/tmp` keeps the repository clean.
  Date/Author: 2026-06-06 / Codex

- Decision: Keep `tracker-export-check` as a Python-only target and run it inside the existing builder image for verification.
  Rationale: The builder image already supports Python and is the right environment for repository build targets. The browser smoke check requires Docker-on-host rather than the builder container.
  Date/Author: 2026-06-06 / Codex

## Outcomes & Retrospective

Implemented the build-time tracker export slice. Maintainers now have explicit `make tracker-export`, `make tracker-export-check`, and `make tracker-export-smoke` commands. The default ROM build remains unchanged; these are utility targets only.

Validation passed in the Docker builder for export/check, in Playwright Docker for browser smoke, and in the Docker builder for `make -j"$(nproc)" NO_MULTIBOOT=1`. The browser smoke loaded the generated `SETDEX_PK`, `PARTY_ORDER_PK`, and `LOCATIONS` constants in Chromium and verified Sawyer plus Route 101.

## Context and Orientation

`tools/tracker_export/export_tracker_data.py` reads source tracker data from `src/data/trainers.party`, `src/data/wild_encounters.json`, `data/maps/*/map.json`, and `src/data/region_map/region_map_sections.json`. It writes generated artifacts into `build/tracker_export/`, which is ignored by git. The generated files are `tracker_data.json`, `pkcalc/sets.js`, `pkcalc/party_order.js`, and `pkcalc/locations.js`.

`tools/tracker_export/validate_tracker_export.py` runs the exporter, loads the generated JSON, unwraps the generated JavaScript constants, and spot-checks Hiker Sawyer and Route 101. `SETDEX_PK` is the generated trainer set object, `PARTY_ORDER_PK` is the generated trainer party-order object, and `LOCATIONS` is the generated route/location object.

The Makefile has a `RULES_NO_SCAN` variable for utility targets that should not trigger the normal tool-building and dependency-scanning setup. The tracker export targets belong there because they are explicit data-export commands, not ROM build prerequisites.

## Plan of Work

Add Makefile variables for the tracker export directory, tracker tool directory, Playwright Docker image, and Playwright npm package. Add `tracker-export`, `tracker-export-check`, and `tracker-export-smoke` to `.PHONY` and `RULES_NO_SCAN`.

Add `tools/tracker_export/smoke_tracker_export_playwright.cjs`. This script takes `--output-dir`, loads the generated adapter files into a Chromium page with Playwright, and asserts that `SETDEX_PK`, `PARTY_ORDER_PK`, and `LOCATIONS` are present. It checks Sawyer's Geodude set, Sawyer's party order, Route 101 coordinate `[4, 10]`, and Route 101 grass encounters.

Update `tools/tracker_export/README.md` with the Makefile commands and the fact that the browser smoke target uses Docker. Update `PATCH_NOTES.md` for the source and documentation changes.

## Concrete Steps

Work from the repository root:

    cd /home/bayesartre/dev/pokeemerald-expansion-shared-power

Generate tracker data:

    make tracker-export

Validate tracker data:

    make tracker-export-check

Run the browser smoke check through Docker:

    make tracker-export-smoke

Run the tracker export and check targets inside the builder image:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make tracker-export tracker-export-check

Run the ROM build because the Makefile is touched:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1

## Validation and Acceptance

Acceptance requires all of the following evidence. `make tracker-export` creates `build/tracker_export/tracker_data.json`, `build/tracker_export/pkcalc/sets.js`, `build/tracker_export/pkcalc/party_order.js`, and `build/tracker_export/pkcalc/locations.js`.

`make tracker-export-check` exits successfully and prints that `tracker_data.json` validated and that spot checks passed for `TRAINER_SAWYER_1`, Route 101 encounters, and Route 101 coordinates.

`make tracker-export-smoke` exits successfully and prints a JSON object with `"status": "ok"` and true checks for `setdexLoaded`, `partyOrderLoaded`, `locationsLoaded`, `sawyerGeodude`, `sawyerParty`, `route101Coord`, and `route101Grass`.

The Docker builder command for `make tracker-export tracker-export-check` exits successfully. The Docker builder command for `make -j"$(nproc)" NO_MULTIBOOT=1` exits successfully. The default `make` target is not changed to depend on tracker export.

## Idempotence and Recovery

The export targets are safe to rerun. They write only under `build/tracker_export/`. If generated data looks stale or validation fails, delete `build/tracker_export/` and rerun `make tracker-export-check`. The Playwright smoke target installs npm dependencies only inside the temporary Docker container and does not add `node_modules` to the repository.

## Artifacts and Notes

Local target checks:

    make tracker-export
    Wrote build/tracker_export/tracker_data.json
    Wrote build/tracker_export/pkcalc/sets.js
    Wrote build/tracker_export/pkcalc/party_order.js
    Wrote build/tracker_export/pkcalc/locations.js

    make tracker-export-check
    Validated build/tracker_export/tracker_data.json
    Spot checks passed: TRAINER_SAWYER_1, Route 101 encounters, Route 101 coords

Builder-container export/check:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make tracker-export tracker-export-check
    Validated build/tracker_export/tracker_data.json
    Spot checks passed: TRAINER_SAWYER_1, Route 101 encounters, Route 101 coords

Playwright smoke target:

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
      },
      "counts": {
        "setdexSpecies": 233,
        "partyOrder": 854,
        "locations": 213,
        "route101Encounters": 12
      }
    }

Docker ROM build:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    arm-none-eabi-objcopy -O binary pokeemerald.elf pokeemerald.gba
    tools/gbafix/gbafix pokeemerald.gba -p --silent

## Interfaces and Dependencies

The Makefile targets are:

    tracker-export
    tracker-export-check
    tracker-export-smoke

The Playwright smoke script is:

    tools/tracker_export/smoke_tracker_export_playwright.cjs

The Docker image used by default is:

    mcr.microsoft.com/playwright:v1.55.0-noble

The npm package installed inside that container is:

    playwright@1.55.0

Revision Note (2026-06-06): Initial ExecPlan for explicit tracker export Makefile targets and repeatable Playwright browser smoke verification.

Revision Note (2026-06-06): Recorded completed implementation evidence after builder-container export/check, Playwright smoke, and `NO_MULTIBOOT=1` Docker build passed.
