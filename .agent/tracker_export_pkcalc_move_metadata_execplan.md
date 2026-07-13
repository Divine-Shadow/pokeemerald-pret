# Generate PKCalc move metadata catalog proofs

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This document must be maintained in accordance with `.agent/PLANS.md`.

## Purpose / Big Picture

The tracker export already has PKCalc data-migration proofs for natures, ability identities, and held-item identities. The next useful migration proof is moves, but only for identity and basic metadata: normalized id, display name, move type, move category, and base power. After this change, maintainers can run an opt-in target that generates repo-derived move metadata artifacts under `build/tracker_export/data_migration/`, opens live PKCalc, and writes a report showing exactly which generated moves align with live PKCalc and which fields or ids remain gaps.

This slice does not replace PKCalc's live move catalog, route generated moves into the overlay, migrate species data, or claim move-effect or damage-calculator parity.

## Progress

- [x] (2026-06-07 04:36Z) Created this ExecPlan after inspecting the existing natures, ability, and held-item migration proof tooling, the Makefile targets, `include/constants/moves.h`, `src/data/moves_info.h`, `include/move.h`, `include/constants/pokemon.h`, and `src/data/types_info.h`.
- [x] (2026-06-07 04:40Z) Extended the exporter so it emits move source reports, `pkcalc_moves_by_id.json`, `pkcalc_calc_moves.json`, and `pkcalc_moves.js` from repository data.
- [x] (2026-06-07 04:44Z) Extended the Playwright audit so it compares generated move metadata against live `MOVES_BY_ID` and `calc.MOVES[4]` and writes `move_metadata_gap_report.json`.
- [x] (2026-06-07 04:46Z) Added the opt-in `tracker-export-move-metadata-migration-check` Makefile target and offline validator support.
- [x] (2026-06-07 04:49Z) Ran syntax checks, the new move metadata target, existing migration checks, the full tracker verification chain, and Docker `make NO_MULTIBOOT=1`.

## Surprises & Discoveries

- Observation: Live PKCalc exposes two move surfaces with different basic-power field names.
  Evidence: A Playwright probe on 2026-06-07 read `MOVES_BY_ID` entries shaped like `{kind, id, name, flags, type, basePower}` and `calc.MOVES[4]` entries shaped like `{bp, type, category}`. For example, live `MOVES_BY_ID.tackle` has `basePower: 35`, while `calc.MOVES[4].Tackle` has `bp: 35`.

- Observation: Live `MOVES_BY_ID` does not always include every basic field on every entry.
  Evidence: The same probe showed `MOVES_BY_ID.nomove` includes `category: "Status"`, but `MOVES_BY_ID.tackle` omits `category`; `calc.MOVES[4].Tackle` includes `category: "Physical"`. The move proof must report missing live fields as gaps instead of failing the whole proof for a sparse live entry.

- Observation: The repository move table uses config-dependent preprocessor branches and expressions.
  Evidence: `src/data/moves_info.h` contains fields such as `.power = B_UPDATED_MOVE_DATA >= GEN_6 ? 45 : 35` and `#if B_UPDATED_MOVE_DATA >= GEN_6` blocks. The exporter should read a preprocessed view with repository config headers included, then only place concrete basic values into PKCalc-shaped artifacts.

- Observation: All direct numeric regular move constants can produce complete basic metadata after resolving simple config-dependent expressions.
  Evidence: Running `python3 tools/tracker_export/export_pkcalc_data_migration.py --output-dir build/tracker_export/data_migration` generated 847 move candidates with `metadataCompleteCount` 847 and no source failures.

## Decision Log

- Decision: Export only move identity and basic metadata fields: `kind`, `id`, `name`, `type`, `category`, and `basePower`.
  Rationale: These fields are present in PKCalc's live move surfaces and can be derived from `gMovesInfo`. Move flags, effects, secondary effects, targets, recoil, multi-hit behavior, and other battle semantics require separate calculator logic and are outside this slice.
  Date/Author: 2026-06-07 / Codex

- Decision: Treat live move metadata differences as reportable gaps unless the generated artifact itself is malformed.
  Rationale: The repository uses current expansion data while live PKCalc's Generation 4 calculator surface can contain older values, such as Tackle base power 35. The proof must surface those differences without claiming generated moves are safe to replace live defaults.
  Date/Author: 2026-06-07 / Codex

## Outcomes & Retrospective

Completed. The repository now generates a PKCalc move identity/basic-metadata proof from repo move data. The generated move artifacts include `source_moves.json`, `pkcalc_moves_by_id.json`, `pkcalc_calc_moves.json`, and `pkcalc_moves.js`, all under `build/tracker_export/data_migration/`. The live Playwright audit writes `move_metadata_gap_report.json` and records live compatibility gaps without treating them as damage-calculator parity.

The final live report status is `ok_with_gaps`: 847 generated moves, 847 shared live ids, 847 generated calc entries, 450 shared `calc.MOVES[4]` names, 397 generated moves missing from the Generation 4 calc surface, 110 live-only by-id entries, 23 live-only calc names, 28 missing live fields, and 253 incompatible basic values. These differences are expected evidence for this proof slice, not permission to replace PKCalc defaults.

## Context and Orientation

Tracker migration tooling lives in `tools/tracker_export/`. `tools/tracker_export/export_pkcalc_data_migration.py` generates JSON and JavaScript proof artifacts from repository source data. `tools/tracker_export/pkcalc_data_migration_playwright.cjs` opens the live PKCalc app and writes reports based on live global catalogs. `tools/tracker_export/validate_pkcalc_data_migration.py` validates generated reports offline. The Makefile target `tracker-export-data-migration-shape-audit` runs the exporter and live audit; existing opt-in checks validate natures and ability/held-item identity proofs.

The repository source of truth for move metadata is `src/data/moves_info.h`, specifically `gMovesInfo[MOVES_COUNT_ALL]`. Numeric move constants are in `include/constants/moves.h`. Move type constants are in `include/constants/pokemon.h`, and display names for types are in `src/data/types_info.h`. Move categories are `DAMAGE_CATEGORY_PHYSICAL`, `DAMAGE_CATEGORY_SPECIAL`, and `DAMAGE_CATEGORY_STATUS`.

A "move metadata catalog" in this plan means a generated PKCalc-id-keyed object whose values contain `kind: "Move"`, `id`, `name`, `type`, `category`, and `basePower`. It does not mean a full move behavior database. The generated artifacts are proof data only and must stay under `build/tracker_export/data_migration/`.

## Plan of Work

Extend `tools/tracker_export/export_pkcalc_data_migration.py` to parse move constants and a preprocessed view of `src/data/moves_info.h` with `include/config/general.h` and `include/config/battle.h` included. The source report should record repository fields such as constant, numeric index, display name, PKCalc id, type constant, category constant, power, effect, accuracy, pp, target, priority, and selected deferred semantic fields for traceability. The PKCalc-shaped artifacts should include only complete entries with concrete `name`, `type`, `category`, and `basePower`.

Extend `tools/tracker_export/pkcalc_data_migration_playwright.cjs` so it loads generated move artifacts, flattens live `MOVES_BY_ID`, reads `calc.MOVES[4]`, and writes `move_metadata_gap_report.json`. The report must include shared ids, generated moves missing from live `MOVES_BY_ID`, generated moves missing from `calc.MOVES[4]`, live-only ids, live-only calc names, missing fields, incompatible shared values, and deferred semantic fields. The report may have status `ok_with_gaps` when differences are recorded but generated artifacts are valid.

Extend `tools/tracker_export/validate_pkcalc_data_migration.py` and the Makefile with an opt-in `tracker-export-move-metadata-migration-check` target. The offline validator should require valid source and generated move artifacts, a gap report with no structural failures, and explicit counts for reported gaps.

Update `tools/tracker_export/README.md` and `PATCH_NOTES.md`. Do not wire generated move artifacts into the overlay or release bundle as defaults.

## Concrete Steps

Run commands from `/home/bayesartre/dev/pokeemerald-expansion-shared-power`.

After implementation, run:

    python3 -m py_compile tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/validate_pkcalc_data_migration.py tools/tracker_export/export_pkcalc_bundle.py tools/tracker_export/validate_pkcalc_bundle.py
    node --check tools/tracker_export/pkcalc_data_migration_playwright.cjs
    git diff --check -- Makefile PATCH_NOTES.md tools/tracker_export/README.md tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/pkcalc_data_migration_playwright.cjs tools/tracker_export/validate_pkcalc_data_migration.py .agent/tracker_export_pkcalc_move_metadata_execplan.md
    make tracker-export-move-metadata-migration-check
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

The exporter is accepted when it writes `source_moves.json`, `pkcalc_moves_by_id.json`, `pkcalc_calc_moves.json`, `pkcalc_moves.js`, and the existing migration artifacts under `build/tracker_export/data_migration/`, and exits zero only when source extraction has no structural failures.

The Playwright audit is accepted when it opens live PKCalc, reads `MOVES_BY_ID` and `calc.MOVES[4]`, writes `move_metadata_gap_report.json`, and records metadata gaps without treating expected repo/live value differences as replacement-catalog success.

The full slice is accepted only after the new opt-in move metadata migration check, existing tracker checks, compatibility and bundle checks, and Docker ROM build pass or any failure is captured here with reproducible evidence.

## Idempotence and Recovery

All generated move migration artifacts are under `build/tracker_export/data_migration/` and can be regenerated safely. The Playwright Docker command installs Node dependencies in a temporary container directory and does not write `node_modules` into the repository.

If the live PKCalc app is unavailable, rerun the Playwright-backed targets when network access recovers. If PKCalc changes move field names, the gap report should identify missing fields or incompatible shared values so maintainers can update mappings intentionally.

If Docker ROM builds fail with `tools/gbafix/gbafix: No such file or directory`, inspect whether host tools were built with a Nix interpreter and rebuild them inside the builder image with:

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -f make_tools.mk clean-tools tools CC=/usr/bin/gcc CXX=/usr/bin/g++ LDFLAGS=

## Artifacts and Notes

Final evidence will be recorded here after verification.

Final verification evidence:

    python3 -m py_compile tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/validate_pkcalc_data_migration.py tools/tracker_export/export_pkcalc_bundle.py tools/tracker_export/validate_pkcalc_bundle.py

    node --check tools/tracker_export/pkcalc_data_migration_playwright.cjs

    git diff --check -- Makefile PATCH_NOTES.md tools/tracker_export/README.md tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/pkcalc_data_migration_playwright.cjs tools/tracker_export/validate_pkcalc_data_migration.py .agent/tracker_export_pkcalc_move_metadata_execplan.md

    for f in tools/tracker_export/export_pkcalc_data_migration.py tools/tracker_export/pkcalc_data_migration_playwright.cjs tools/tracker_export/validate_pkcalc_data_migration.py .agent/tracker_export_pkcalc_move_metadata_execplan.md; do out=$(git diff --check --no-index /dev/null "$f" 2>&1 || true); if [ -n "$out" ]; then printf '%s\n' "$out"; exit 1; fi; done

    make tracker-export-move-metadata-migration-check
    PKCalc move metadata migration proof passed: 847 generated moves (ok_with_gaps), 847 shared live ids, 253 incompatible values reported

    make tracker-export-natures-migration-check
    PKCalc natures migration proof passed: 25 source natures, 25 by-id entries, 25 calc entries

    make tracker-export-identity-migration-check
    PKCalc identity migration proof passed: 310 abilities (ok_with_gaps), 323 held items (ok_with_gaps)

    make tracker-export && make tracker-export-check && make tracker-export-coverage-check && make tracker-export-reference-check && make tracker-export-live-reference-check && make tracker-export-overlay-check && make tracker-export-bundle-check && make tracker-export-compat-check
    Coverage audit passed: no missing or unexpected tracker records
    Reference audit passed: no malformed, missing, unexpected, or conflicting references
    Live reference audit passed: species: 233, moves: 266, abilities: 81, items: 48, natures: 17, encounterSpecies: 132
    Bundle checks passed: overlay paths, manifest, coverage, references, live references, contract, checksums, VERIFY.txt
    tracker-export-compat-check status: ok

    docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)" NO_MULTIBOOT=1
    ROM build completed and wrote pokeemerald.gba

Revision Note (2026-06-07): Recorded implementation and verification outcomes for the completed move metadata proof slice.

Revision Note (2026-06-07): Initial plan for a move identity/basic metadata migration proof, scoped to generated proof artifacts and explicit live PKCalc gap reports.
