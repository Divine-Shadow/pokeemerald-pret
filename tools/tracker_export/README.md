# Tracker Export MVP

This directory contains a deterministic static-data exporter for PKCalc-style tracker data.

Run from the repository root:

    make tracker-export
    make tracker-export-check
    make tracker-export-overlay
    make tracker-export-overlay-check
    make tracker-export-smoke
    make tracker-export-site-smoke

The equivalent direct Python commands are:

    python3 tools/tracker_export/export_tracker_data.py --output-dir build/tracker_export
    python3 tools/tracker_export/validate_tracker_export.py --output-dir build/tracker_export

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

The generated JavaScript files are build artifacts. This repository ignores `*.js`, so the Python exporter and this README are the maintained source files.

`make tracker-export-smoke` uses the official Playwright Docker image to load the generated PKCalc adapter files in Chromium. It installs the matching `playwright` npm package in a temporary directory inside the container, then runs `tools/tracker_export/smoke_tracker_export_playwright.cjs`.

`make tracker-export-site-smoke` opens the live PKCalc app in Chromium and intercepts its requests for `js/data/party_order.js`, `js/data/sets.js`, and `js/data/dex/locations.js`, serving this repo's generated files instead. It then checks PKCalc's own set-option and location-rendering paths for Sawyer's Geodude and Route 101. Override the app URL with `PKCALC_URL=...` when needed.

The overlay directory is a handoff artifact. Copy the contents of `build/tracker_export/pkcalc_overlay/` over a PKCalc build root to replace only:

- `js/data/party_order.js`
- `js/data/sets.js`
- `js/data/dex/locations.js`

`manifest.json` records the source commit, dirty-worktree flag, source files, generated paths, and record counts.

## Scope

This MVP covers static tracker data only: trainer sets, trainer party order, location grid coordinates, and wild encounter tables. It intentionally does not export the full species, moves, items, abilities, or damage-calculator database, and it does not implement Lua/save sync or emulator integration.
