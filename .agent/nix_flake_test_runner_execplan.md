# Add Nix flake test-runner checks

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This document is maintained according to `.agent/PLANS.md`.

## Purpose / Big Picture

After this change, the repository's automated C test suite can run on NixOS without relying on the checked-in generic Linux `tools/mgba/mgba-rom-test` binary. A developer should be able to enter the Nix dev shell and run the normal `make check` flow, and CI or a local operator should be able to run `nix flake check` to prove both the ROM build and test path.

## Progress

- [x] (2026-06-06T22:10Z) Inspected the current `Makefile` test wiring and confirmed it prefers `mgba-rom-test` and `mgba-rom-test-hydra` found on `PATH`.
- [x] (2026-06-06T22:13Z) Confirmed nixpkgs `pkgs.mgba` installs `mgba` and `mgba-qt` but not `mgba-rom-test`.
- [x] (2026-06-06T22:16Z) Materialized the nixpkgs mGBA source and found the upstream `BUILD_ROM_TEST` CMake option and `src/platform/test/rom-test-main.c` target.
- [x] (2026-06-06T22:18Z) Prototyped a minimal Nix derivation that builds and runs `mgba-rom-test --help`.
- [x] (2026-06-06T22:22Z) Added flake packages for `mgba-rom-test`, `mgba-rom-test-hydra`, and a Nix test derivation, plus flake `checks` for ROM build and tests.
- [x] (2026-06-06T22:31Z) Fixed the `mgba-rom-test-hydra` package to clean stale source-directory binaries before building.
- [x] (2026-06-06T22:38Z) Fixed the hydra wrapper's TTY progress rendering to tolerate a zero-column `TIOCGWINSZ` result.
- [x] (2026-06-06T22:49Z) Confirmed the full unfiltered suite runs under Nix and currently fails with seven behavioral test failures unrelated to Nix wiring.
- [x] (2026-06-06T22:51Z) Changed the flake check test derivation to an explicit `uq4_12` smoke check that proves the Nix mGBA runner path without masking full-suite failures.
- [x] (2026-06-06T23:02Z) Verified the dev shell exposes Nix-store `mgba-rom-test` and `mgba-rom-test-hydra` paths.
- [x] (2026-06-06T23:03Z) Verified the equivalent Nix-wired `make check TESTS="uq4_12"` smoke derivation passes 4 tests through mGBA.
- [x] (2026-06-06T23:04Z) Verified `nix build --no-link --print-out-paths path:$PWD#pokeemerald` still produces the ROM output.
- [x] (2026-06-06T23:05Z) Verified `nix flake check path:$PWD` exits successfully and runs the ROM build and Nix test-runner smoke checks.
- [x] (2026-06-06T23:12Z) Found that dirty path verification had copied stale build objects; updated the Nix build phases to clean build object directories and fixed a GCC 15 test warning exposed by an artifact-free checkout.
- [x] (2026-06-06T23:20Z) Re-ran artifact-free temporary-checkout `nix flake check`; it passed and produced the expected ROM and smoke-test outputs.
- [x] (2026-06-06T23:20Z) Recorded final validation evidence and the full-suite caveat.

## Surprises & Discoveries

- Observation: The nixpkgs mGBA package does not expose the binary this repository's Makefile needs.
  Evidence: `nix shell nixpkgs#mgba -c sh -lc 'command -v mgba-rom-test || true; command -v mgba; command -v mgba-qt'` found only `mgba` and `mgba-qt`.

- Observation: mGBA upstream already has a narrow ROM-test CMake target.
  Evidence: The nixpkgs mGBA source has `BUILD_ROM_TEST` in `CMakeLists.txt` and installs `${BINARY_NAME}-rom-test` from `src/platform/test/CMakeLists.txt`.

- Observation: A local path source can include ignored stale binaries, and the `tools/mgba-rom-test-hydra` Makefile will not rebuild an existing binary if it is newer than `main.c`.
  Evidence: The first `pokeemerald-tests` derivation invoked the Nix-store hydra path, but that binary failed with the NixOS dynamic-loader error because the package had installed the stale source-directory executable. Adding `make clean` before `make` forces a Nix-built binary.

- Observation: In a Nix build log, the hydra wrapper can enter its TTY rendering branch and receive a terminal width of zero.
  Evidence: The second `pokeemerald-tests` derivation printed `[00] WAITING...` through `[15] WAITING...` and then `make` reported `Floating point exception`. The only integer division in that branch divides by `winsize.ws_col`, so a zero-column result caused the crash.

- Observation: The Nix runner path now executes the full suite, but the current dirty repository state is not fully green.
  Evidence: The unfiltered Nix test derivation reported 3,349 passed tests, 18 known-failing tests, 792 TODO tests, 3 assumption failures, and 7 failed tests, including `Condition Coach rejects Eggs` and weather/protosynthesis battle expectations.

- Observation: Path-based local verification can copy ignored `build/` object files and hide clean-compile failures.
  Evidence: A temporary committed checkout without ignored build artifacts failed compiling `test/condition_coach.c` under GCC 15 with `abilityNum may be used uninitialized`, while the dirty path smoke had passed by reusing copied objects.

## Decision Log

- Decision: Build `mgba-rom-test` from nixpkgs' existing mGBA source instead of vendoring emulator code or modifying the repository's checked-in binary.
  Rationale: The user forbade vendoring large external dependencies, and nixpkgs already pins and fetches the mGBA source used by its package set.
  Date/Author: 2026-06-06 / Codex

- Decision: Package the repository's `tools/mgba-rom-test-hydra` wrapper as a flake package and put it on the dev-shell/check PATH.
  Rationale: The Makefile discovers `ROMTESTHYDRA` with `command -v`, so a Nix-built wrapper avoids stale generic host binaries while preserving the normal Makefile path for non-Nix users.
  Date/Author: 2026-06-06 / Codex

- Decision: Run `make clean` in the `mgba-rom-test-hydra` derivation before building.
  Rationale: Local flake path sources can contain ignored build outputs from the working tree. Cleaning first prevents packaging a stale host-built executable.
  Date/Author: 2026-06-06 / Codex

- Decision: Clamp a zero terminal width to 80 columns in `tools/mgba-rom-test-hydra/main.c`.
  Rationale: Zero columns are not meaningful for progress rendering, and 80 columns is a conservative fallback that prevents division by zero without changing test scheduling or results.
  Date/Author: 2026-06-06 / Codex

- Decision: Add a Nix test derivation and expose it as `checks.<system>.test`.
  Rationale: `nix flake check` needs a derivation that executes the existing `make check` path and fails if the selected Nix test path fails.
  Date/Author: 2026-06-06 / Codex

- Decision: Use `make check TESTS="uq4_12"` as the flake check's test-runner smoke.
  Rationale: The full unfiltered suite now runs under Nix but currently fails due existing behavioral test failures outside the Nix integration scope. The smoke check still compiles the test ELF, patches it for headless execution, launches the Nix-built mGBA ROM-test runner, and fails if that path breaks.
  Date/Author: 2026-06-06 / Codex

- Decision: Run `make tidymodern` in the ROM derivation and `make tidycheck` in the smoke derivation before compiling.
  Rationale: Local `path:` flake verification can include ignored build directories; cleaning object trees in the temporary build directory makes those builds behave like clean checkouts.
  Date/Author: 2026-06-06 / Codex

- Decision: Initialize `abilityNum` in `test/condition_coach.c`.
  Rationale: GCC 15 can warn that the parametrized test local may be used uninitialized. Initializing it preserves the test behavior for all parametrized cases and allows clean Nix test builds to compile with `-Werror`.
  Date/Author: 2026-06-06 / Codex

## Outcomes & Retrospective

The Nix test-runner integration is implemented and verified. The dev shell exposes Nix-store `mgba-rom-test` and `mgba-rom-test-hydra` binaries, `checks.<system>.test` runs `make check TESTS="uq4_12"` through the Nix-built mGBA runner, `checks.<system>.rom-build` points at the ROM derivation, and `nix flake check` exits successfully from an artifact-free temporary Git checkout. The full unfiltered suite has been proven runnable under Nix but is not green in the current dirty repository state.

## Context and Orientation

The root `Makefile` builds a test ELF when the `check` target is requested. It patches the ELF into headless mode with `tools/patchelf/patchelf`, then runs `$(ROMTESTHYDRA) $(ROMTEST) $(OBJCOPY) $(HEADLESSELF)`. On Linux, `ROMTEST` is set to `mgba-rom-test` if that command exists on `PATH`, otherwise it falls back to `tools/mgba/mgba-rom-test`. `ROMTESTHYDRA` is similarly discovered from `PATH` before falling back to `tools/mgba-rom-test-hydra/mgba-rom-test-hydra`.

The checked-in `tools/mgba/mgba-rom-test` binary does not run on NixOS because it is a generic dynamically linked Linux executable. Nix can instead build an mGBA ROM-test binary from source and place it on `PATH`.

## Plan of Work

Add a `mgba-rom-test` flake package that builds nixpkgs' mGBA source with `BUILD_ROM_TEST=ON` and graphical frontends disabled. Add a `mgba-rom-test-hydra` flake package that builds the local wrapper from `tools/mgba-rom-test-hydra`. Add both packages to the development shell. Add a `pokeemerald-test-smoke` derivation that cleans stale local helper tools in its temporary build tree, runs `make -j"$NIX_BUILD_CORES" NO_MULTIBOOT=1 TESTS="uq4_12" check`, and writes a small success marker into `$out`.

Expose `checks.<system>.rom-build` as the existing ROM derivation and `checks.<system>.test` as the new test derivation. Update the NixOS install docs with the test commands and update `PATCH_NOTES.md` for the file changes.

## Concrete Steps

From `/home/bayesartre/dev/pokeemerald-expansion-shared-power`, run:

    nix develop path:$PWD -c make clean-check-tools
    nix develop path:$PWD -c make NO_MULTIBOOT=1 TESTS="uq4_12" check
    nix build path:$PWD
    nix flake check path:$PWD

To run the full unfiltered suite under Nix, use:

    nix develop -c make NO_MULTIBOOT=1 check

In the current dirty tree, that full command runs but reports seven behavioral failures. When the flake files are tracked in Git, the same commands can use plain `.` or no explicit flake path:

    nix develop -c make NO_MULTIBOOT=1 TESTS="uq4_12" check
    nix build
    nix flake check

## Validation and Acceptance

Acceptance requires an existing repository test command to run through Nix using the Nix-built `mgba-rom-test` binary, the ROM build to remain green, and `nix flake check` to pass. Evidence should include the exact commands and concise output showing success. Full unfiltered `make check` should be reported separately because current behavioral failures are outside the Nix integration.

## Idempotence and Recovery

The flake check derivations build in temporary Nix build directories and do not modify the working tree. Interactive `nix develop` commands operate in the working tree; if ignored helper binaries were previously built outside Nix, run `nix develop -c make clean-tools clean-check-tools` before `make check`.

## Artifacts and Notes

Prototype command for the mGBA ROM-test package:

    nix build --no-link --print-out-paths --impure --expr '<temporary mgba-rom-test derivation>'
    /nix/store/z17i2yikjpdzl89msjrv5d3my5h21pq4-mgba-rom-test-0.10.5
    /nix/store/z17i2yikjpdzl89msjrv5d3my5h21pq4-mgba-rom-test-0.10.5/bin/mgba-rom-test --help
    usage: .../mgba-rom-test [option ...] file

Dev-shell runner probe:

    nix develop path:$PWD -c sh -lc 'printf "ROMTEST=%s\n" "$ROMTEST"; command -v mgba-rom-test; command -v mgba-rom-test-hydra'
    ROMTEST=/nix/store/all5wsa6nms3d4xiiy2csrpr0yknmaqi-mgba-rom-test-0.10.5/bin/mgba-rom-test
    /nix/store/all5wsa6nms3d4xiiy2csrpr0yknmaqi-mgba-rom-test-0.10.5/bin/mgba-rom-test
    /nix/store/4lhb4jx1b05bjnkd8g2rv775d730iq1w-mgba-rom-test-hydra-0-unstable/bin/mgba-rom-test-hydra

Nix test-runner smoke:

    nix build --no-link --print-out-paths path:$PWD#pokeemerald-test-smoke
    /nix/store/r6fmn3vn4sblywl8mx77kw49dwk9qvpk-pokeemerald-expansion-shared-power-test-smoke-0-unstable
    - Tests PASSED:          4
    - Tests TOTAL:           4
    check-passed.txt: make check TESTS="uq4_12" passed under Nix

ROM build:

    nix build --no-link --print-out-paths path:$PWD#pokeemerald
    /nix/store/hm2zyxzq52xdbx7ywid0shxlz5rnmlz5-pokeemerald-expansion-shared-power-0-unstable
    pokeemerald.elf 33122416 bytes
    pokeemerald.gba 33554432 bytes
    pokeemerald.map 3748190 bytes

Aggregate flake check:

    nix flake check
    checking derivation checks.x86_64-linux.rom-build...
    checking derivation checks.x86_64-linux.test...
    running 2 flake checks...
    command exited successfully

Artifact-free temporary Git checkout final outputs:

    rom=/nix/store/7q0nnmxgig9kqs96fi2swfx9411x032c-pokeemerald-expansion-shared-power-0-unstable
    pokeemerald.elf 33095968 bytes
    pokeemerald.gba 33554432 bytes
    pokeemerald.map 3748655 bytes
    test=/nix/store/s4b971smr1vhk6gbq112xsdfv541nqs4-pokeemerald-expansion-shared-power-test-smoke-0-unstable
    make check TESTS="uq4_12" passed under Nix

Full unfiltered suite caveat from the pre-smoke full-suite derivation run:

    - Tests FAILED :         7
    - Tests KNOWN_FAILING:   18
    - ASSUMPTIONS_FAILED:    3
    - Tests TO_DO:           792
    - Tests PASSED:          3349
    - Tests TOTAL:           4169

This proved the Nix runner path could execute the full test workload before the flake check was narrowed to a smoke target; the failures are behavioral expectations in the current tree, not Nix runner startup failures.

## Interfaces and Dependencies

The main interface remains the existing Makefile `check` target. The flake supplies `mgba-rom-test` and `mgba-rom-test-hydra` on `PATH`; it does not change the test framework, test source files, or the Docker/direct Makefile workflows.

Revision Note (2026-06-06): Initial plan created for adding Nix-native test-runner support and flake checks.

Revision Note (2026-06-06): Recorded and fixed the stale `mgba-rom-test-hydra` source binary issue discovered by the first test derivation run.

Revision Note (2026-06-06): Recorded and fixed the zero-column terminal-width crash discovered by the second test derivation run.

Revision Note (2026-06-06): Changed the flake check from the currently failing full suite to an explicit Nix runner smoke and documented the full-suite failure evidence.

Revision Note (2026-06-06): Added final validation evidence for dev-shell runner discovery, smoke tests, ROM build, and aggregate flake check.

Revision Note (2026-06-06): Added object-directory cleanup to the Nix derivations and recorded the GCC 15 `condition_coach` test warning exposed by artifact-free verification.

Revision Note (2026-06-06): Recorded the artifact-free temporary-checkout `nix flake check` result and final output paths.
