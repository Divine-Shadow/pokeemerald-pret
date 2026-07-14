# Repository Guidelines

## Mandatory: Update `PATCH_NOTES.md`
- After every change you make (code, data, or docs), add a concise entry to `PATCH_NOTES.md` at the top of the file.
- Use the format: `- Area: short description (commit <short-hash>)` when a stable commit hash is already known.
- For local work that has not been committed yet, use `(commit pending)`. Do not leave verified work uncommitted just to wait for a perfect rebased hash.
- Example: `- Gameplay: Strength is now a Rock-type move (commit c78da96b3a).`
- Keeping this file current is required for smooth reviews and release notes.

## Project Structure & Module Organization
- Source code: `src/` (C) and headers in `include/`.
- Engine data: `data/`, `constants/`, `graphics/`, `sound/`, and `asm/` for assembly.
- Tests: `test/` (C tests using the in-repo test runner).
- Tools: `tools/` (auto-built by `make`), linker scripts `ld_script_*.ld`.
- Build artifacts: `build/modern*/` and final ROM `pokeemerald.gba`.
- Documentation: `docs/` (tutorials live under `docs/tutorials/`).

## Build, Test, and Development Commands
- `make` or `make rom`: Build tools, generate assets, and produce `pokeemerald.gba`.
- `make check`: Build and run the test suite via mGBA test runner.
- `make clean`: Remove artifacts; `make tidy` and `make tidycheck` for object cleanup.
- Useful flags: `DEBUG=1` (debug info), `TEST=1` (test objects), `LTO=1` (link-time opt), `COMPARE=1` (verify `rom.sha1`). Example: `make DEBUG=1`.
- Symbols/map: `make syms` (creates `.sym`), map at `pokeemerald.map`.

### Docker Builds (recommended for agents)
- Image build: `docker build -t pokeemerald-expansion:builder .`
- Bind-mount build (Linux/macOS): `docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make -j"$(nproc)"`
- Bind-mount tests: `docker run --rm -u "$(id -u):$(id -g)" -v "$PWD:/workspace" -w /workspace pokeemerald-expansion:builder make check`
- Windows PowerShell build: `docker run --rm -v ${PWD}:/workspace -w /workspace pokeemerald-expansion:builder make -j4`
- Note: If you don’t have multiboot blobs, append `NO_MULTIBOOT=1` to `make`.

The Dockerfile provides a reproducible toolchain. Using the bind-mount approach ensures artifacts like `pokeemerald.gba` are written to your host workspace.

## Coding Style & Naming Conventions
- Language: C (GNU17). Follow existing file layout and patterns; see `docs/STYLEGUIDE.md`.
- Indentation: C/C headers use 4 spaces; ASM (`.s`) and scripts (`.inc`) use tabs.
- Naming: functions/structs `PascalCase`; locals/fields `camelCase`; constants/macros `UPPER_SNAKE_CASE`.
- Globals: prefix globals with `g` and file‑static variables with `s`.
- Files: lowercase with underscores (e.g., `src/battle_main.c`, `include/pokemon.h`).
- Warnings: treat as errors (`-Werror`). Keep changes minimal and consistent.

## Testing Guidelines
- Frameworks: unit tests via `TEST("Suite/Case") { ... }` and `EXPECT_*` in `include/test/test.h`; battle tests via the DSL (`SINGLE_BATTLE_TEST`, `DOUBLE_BATTLE_TEST`, `AI_SINGLE_BATTLE_TEST`, etc.).
- Location: place new tests under `test/` (mirror engine areas, e.g., `test/battle/...`).
- Run: `make check` (filter with `TESTS="substring"` or via runner args; see `test/test_runner.c`).
- Philosophy: deterministic, fast, isolated; prefer observable outputs (messages/animations/HP bar) over internal state.

## Commit & Pull Request Guidelines
- Commits: concise, imperative mood; reference PR/issue when relevant (e.g., `Fix Wonder Room interaction (#7626)`).
- Once your scoped work is implemented and verified, prefer committing it so the tree is clean for the next agent or reviewer. A clean tree with `commit pending` notes is better than an uncommitted verified change waiting for an exact final hash.
- Branches: target `master` for hotfixes; `upcoming` for features/regular work.
- PRs: clear description, rationale, reproducible steps, linked issues, and screenshots/logs when UI/visuals apply. Keep diffs focused.

## Security & Configuration Tips
- Do not commit ROMs or proprietary assets. Generated outputs (e.g., `.gba`) are artifacts.
- Configure `DEVKITARM` in your environment; follow `INSTALL.md` for setup.
- Features and toggles live under `include/config/`—document changes when modifying configuration.
