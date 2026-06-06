# Tracker Export MVP

This directory contains a deterministic static-data exporter for PKCalc-style tracker data.

Run from the repository root:

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

The generated JavaScript files are build artifacts. This repository ignores `*.js`, so the Python exporter and this README are the maintained source files.

## Scope

This MVP covers static tracker data only: trainer sets, trainer party order, location grid coordinates, and wild encounter tables. It intentionally does not export the full species, moves, items, abilities, or damage-calculator database, and it does not implement Lua/save sync or emulator integration.

