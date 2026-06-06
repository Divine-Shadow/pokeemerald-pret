# Build a PKCalc-compatible tracker export MVP

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan is maintained according to `.agent/PLANS.md` from the repository root.

## Purpose / Big Picture

After this change, a maintainer can run one deterministic exporter from the repository root and get static tracker data for a PKCalc-style tracker. The MVP does not try to recreate the full damage calculator database or live emulator synchronization. It proves the valuable static layer first: trainer sets, trainer party order, location coordinates, and wild encounter tables generated from this repo's source files.

The observable result is a `tracker_data.json` file plus PKCalc-shaped JavaScript adapter files in an ignored build directory. A maintainer should be able to inspect `SETDEX_PK`, `PARTY_ORDER_PK`, and `LOCATIONS`, load their object payloads, and spot-check the data against `src/data/trainers.party`, `src/data/wild_encounters.json`, `data/maps/*/map.json`, and `src/data/region_map/region_map_sections.json`.

## Progress

- [x] (2026-06-06 20:36Z) Read `.agent/PLANS.md`, the gameplay updater skill guidance, `PATCH_NOTES.md`, `.gitignore`, and source data examples for trainers, encounters, maps, and region map sections.
- [x] (2026-06-06 20:36Z) Created this ExecPlan and scoped the MVP to static tracker export only.
- [x] (2026-06-06 20:43Z) Implemented a deterministic exporter under `tools/tracker_export/`.
- [x] (2026-06-06 20:43Z) Generated and validated `tracker_data.json`, `sets.js`, `party_order.js`, and `locations.js` under `build/tracker_export/`.
- [x] (2026-06-06 20:43Z) Added narrow validation coverage for JSON shape and PKCalc adapter shape.
- [x] (2026-06-06 20:43Z) Ran the exporter from a fresh temp output directory, validated JSON syntax, and spot-checked one trainer, one route encounter table, and one map coordinate.

## Surprises & Discoveries

- Observation: The repository `.gitignore` ignores all `*.js` files globally.
  Evidence: `.gitignore` contains `*.js`, so PKCalc adapter `.js` files should be generated into `build/tracker_export/` rather than treated as committed source.

- Observation: Trainer data is already written in a Pokemon Showdown-like format.
  Evidence: `src/data/trainers.party` documents the competitive syntax and shows trainers such as `TRAINER_SAWYER_1` with species, item, ability, level, nature, IVs, and move lines.

- Observation: Wild encounters are already JSON, but PKCalc wants flattened encounter entries grouped under location objects.
  Evidence: `src/data/wild_encounters.json` stores map headers with slot lists, while PKCalc `LOCATIONS` entries use encounter records with `species`, `chance`, `minLevel`, `maxLevel`, and `method`.

- Observation: Some region map sections have no coordinate rectangle, and some share display names.
  Evidence: `src/data/region_map/region_map_sections.json` includes entries such as `MAPSEC_INSIDE_OF_TRUCK`, `MAPSEC_SECRET_BASE`, and Kanto sections without `x`, `y`, `width`, or `height`; clone names such as Route 4 and Route 10 can also repeat.

- Observation: `src/data/wild_encounters.json` contains non-map encounter groups.
  Evidence: `gBattlePyramidWildMonHeaders` has `for_maps: false` and no `fields` array, so the MVP exporter skips non-map groups.

## Decision Log

- Decision: Generate ignored PKCalc adapter `.js` files instead of committing generated JavaScript.
  Rationale: The repository ignores `*.js` globally and already treats generated files as build artifacts. The stable source should be the exporter and its canonical JSON schema.
  Date/Author: 2026-06-06 / Codex

- Decision: Scope the MVP to trainer sets, party order, locations, and wild encounters, deferring full species, move, item, ability, damage-calc, Lua, save, and emulator work.
  Rationale: These sources are enough to prove tracker usefulness and are explicitly inside the user's static-data scope. Full Dex and live sync require parsing config-gated C tables or memory layouts and would expand risk beyond the MVP.
  Date/Author: 2026-06-06 / Codex

- Decision: Collapse map data by `region_map_section` for the MVP `LOCATIONS` output.
  Rationale: PKCalc's map view is a region-grid tracker. Region map sections already provide grid coordinates and names, while many map JSON files share a section for interiors or submaps.
  Date/Author: 2026-06-06 / Codex

- Decision: Use the `MAPSEC_*` id, not the display name, as the source for each location id.
  Rationale: This preserves IDs such as `route101`, avoids collisions from cloned display names, and still keeps names human-readable in the location payload.
  Date/Author: 2026-06-06 / Codex

- Decision: Skip wild encounter groups whose `for_maps` flag is false.
  Rationale: Battle facility encounter pools are not map-backed route/location data and do not include the slot metadata needed for PKCalc-style location encounters.
  Date/Author: 2026-06-06 / Codex

## Outcomes & Retrospective

Implemented `tools/tracker_export/export_tracker_data.py`, `tools/tracker_export/validate_tracker_export.py`, and `tools/tracker_export/README.md`.

The exporter writes a canonical `tracker_data.json` and three PKCalc-shaped adapter files. The generated canonical data currently contains 855 trainer records, 233 setdex species keys, 854 party-order entries, 213 locations, and 12 Route 101 encounter slots. `PARTY_ORDER_PK` has one fewer entry than `trainers` because trainers without parsed party Pokemon are preserved in `trainers` but omitted from party order.

The MVP remains intentionally static. Full Dex export, damage-calc correctness, Lua integration, save sync, and emulator integration are documented as post-MVP gaps in the generated JSON and README.

## Context and Orientation

PKCalc is a browser calculator/tracker that reads global JavaScript constants. For this MVP, `SETDEX_PK` means an object keyed by species display name, where each species contains named trainer sets. `PARTY_ORDER_PK` means an object keyed by the same trainer labels, where each value is the trainer's party species names in order. `LOCATIONS` means an object keyed by location id; each location has a display name, region map coordinates, optional sublocations, and flattened encounter entries.

The source trainer data is `src/data/trainers.party`. It is processed by `tools/trainerproc/main.c` for game builds, but the exporter can parse the source text directly for MVP fields because the file is already Showdown-like. The source wild encounter data is `src/data/wild_encounters.json`. The region-grid coordinates are in `src/data/region_map/region_map_sections.json`. The bridge from map ids such as `MAP_ROUTE101` to region map sections such as `MAPSEC_ROUTE_101` is in `data/maps/<MapName>/map.json`.

Generated files will go under `build/tracker_export/`, which is ignored by git. The planned source files are Python files under `tools/tracker_export/`, because this repository already uses Python for gameplay data helpers such as `tools/wild_encounters/wild_encounters_to_header.py` and `tools/learnset_helpers/make_teachables.py`.

## Plan of Work

First, add `tools/tracker_export/export_tracker_data.py`. It will read trainer, encounter, map, and region-map source files, normalize names and ids deterministically, then write a canonical `tracker_data.json` plus PKCalc adapters. The script should have a stable default output directory of `build/tracker_export` and accept `--output-dir` for testability.

Second, add a narrow validation script or test that runs the exporter into a temporary output directory, loads `tracker_data.json`, strips the `const NAME = ...;` wrapper from each generated JavaScript adapter, and asserts that `SETDEX_PK`, `PARTY_ORDER_PK`, and `LOCATIONS` contain expected records. The test should spot-check `TRAINER_SAWYER_1`, `MAP_ROUTE101`, and the Route 101 region coordinate.

Third, run the exporter and validation commands from the repository root. Record concise evidence in this plan, and update `PATCH_NOTES.md` for the tooling and planning changes.

## Concrete Steps

Work from the repository root:

    cd /home/bayesartre/dev/pokeemerald-expansion-shared-power

Create the exporter:

    python3 tools/tracker_export/export_tracker_data.py --output-dir build/tracker_export

Expected outputs after implementation:

    build/tracker_export/tracker_data.json
    build/tracker_export/pkcalc/sets.js
    build/tracker_export/pkcalc/party_order.js
    build/tracker_export/pkcalc/locations.js

Run the narrow validation command after it exists:

    python3 tools/tracker_export/validate_tracker_export.py --output-dir build/tracker_export

## Validation and Acceptance

Acceptance requires all of the following evidence:

The exporter exits successfully from the repository root and creates `tracker_data.json` plus `pkcalc/sets.js`, `pkcalc/party_order.js`, and `pkcalc/locations.js`.

`tracker_data.json` parses with Python's `json` module. The generated JavaScript adapters have the exact constant names `SETDEX_PK`, `PARTY_ORDER_PK`, and `LOCATIONS`, and their payloads parse as JSON after removing the wrapper.

The trainer spot-check proves that `TRAINER_SAWYER_1` from `src/data/trainers.party` appears as a trainer with a Geodude set whose level is 21, item is Berry Juice, ability is Sturdy, nature is Adamant, and moves include Stealth Rock, Rock Blast, Earthquake, and Sucker Punch.

The route encounter spot-check proves that Route 101 has 12 grass encounter slots derived from `MAP_ROUTE101` land encounters, including Wurmple and Poochyena slots with their source levels and slot chances.

The coordinate spot-check proves that Route 101 has coordinate `[4, 10]`, matching `MAPSEC_ROUTE_101` in `src/data/region_map/region_map_sections.json`.

## Idempotence and Recovery

The exporter must be deterministic and safe to rerun. It writes only under the chosen output directory and creates that directory if needed. If output validation fails, delete `build/tracker_export/` and rerun the exporter after fixing the source script. Since generated adapter files are ignored build artifacts, they should not require git cleanup.

## Artifacts and Notes

Generated artifacts from the repository root:

    python3 tools/tracker_export/export_tracker_data.py --output-dir build/tracker_export
    Wrote build/tracker_export/tracker_data.json
    Wrote build/tracker_export/pkcalc/sets.js
    Wrote build/tracker_export/pkcalc/party_order.js
    Wrote build/tracker_export/pkcalc/locations.js

Validation run against `build/tracker_export/`:

    python3 tools/tracker_export/validate_tracker_export.py --output-dir build/tracker_export
    Validated build/tracker_export/tracker_data.json
    Spot checks passed: TRAINER_SAWYER_1, Route 101 encounters, Route 101 coords

Fresh-output validation run:

    python3 tools/tracker_export/validate_tracker_export.py
    Validated /tmp/tracker-export-7x48ts_n/tracker_data.json
    Spot checks passed: TRAINER_SAWYER_1, Route 101 encounters, Route 101 coords

Syntax check:

    python3 -m py_compile tools/tracker_export/export_tracker_data.py tools/tracker_export/validate_tracker_export.py

## Interfaces and Dependencies

The exporter should use only Python standard-library modules: `argparse`, `json`, `pathlib`, `re`, `shutil` if needed, and collections helpers. It must not depend on PKCalc source being checked out. It should emit PKCalc-compatible shapes based on the inferred interface:

    const SETDEX_PK = { ... };
    const PARTY_ORDER_PK = { ... };
    const LOCATIONS = { ... };

The canonical `tracker_data.json` should contain enough structured source data to regenerate those constants without reading the original game files again. At minimum it should include `schemaVersion`, `trainers`, `setdex`, `partyOrder`, and `locations`.

Revision Note (2026-06-06): Initial ExecPlan created for the static PKCalc tracker export MVP. The plan records the generated-JS build-artifact decision because `.gitignore` ignores JavaScript files globally.
