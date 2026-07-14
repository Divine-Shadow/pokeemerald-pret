# Add a Nix flake ROM build

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This document is maintained according to `.agent/PLANS.md`.

## Purpose / Big Picture

After this change, a developer on NixOS or any machine with Nix flakes enabled can build this repository from source by running `nix build` at the repository root. The build should not require a manually configured host `DEVKITARM`, and the result symlink should contain `pokeemerald.gba` plus useful build side artifacts. The existing Docker and direct `make` workflows must keep working because the flake is additive.

## Progress

- [x] (2026-06-06T21:36Z) Inspected the root `Makefile`, `make_tools.mk`, `Dockerfile`, `check_history.sh`, and existing `docs/install/linux/NIXOS.md` to identify build inputs and history-check behavior.
- [x] (2026-06-06T21:36Z) Confirmed the local nixpkgs has `gcc-arm-embedded` and that it exposes `arm-none-eabi-gcc`, `libnosys.a`, and `libc.a`.
- [x] (2026-06-06T21:36Z) Ran an ad hoc Nix shell build and found stale host-built tool binaries can break NixOS execution unless tools are cleaned and rebuilt inside Nix.
- [x] (2026-06-06T21:36Z) Added an initial `flake.nix`, a NixOS docs update, and this ExecPlan.
- [x] (2026-06-06T21:42Z) Generated `flake.lock` from the selected nixpkgs input, pinned to nixpkgs revision `331800de5053fcebacf6813adb5db9c9dca22a0c`.
- [x] (2026-06-06T21:48Z) Changed the derivation source from `self` to `./.` after temporary path verification exposed source resolution ambiguity.
- [x] (2026-06-06T21:48Z) Verified `nix build path:$PWD` builds the package and outputs `pokeemerald.gba`, `pokeemerald.elf`, and `pokeemerald.map`.
- [x] (2026-06-06T21:49Z) Verified plain `nix build` in a temporary committed Git checkout made from tracked files plus the new flake files.
- [x] (2026-06-06T21:50Z) Verified `nix develop path:$PWD` exposes `DEVKITARM`, `arm-none-eabi-gcc`, and libpng through pkg-config.
- [x] (2026-06-06T21:51Z) Checked `make check` feasibility and recorded why it is outside this flake change.
- [x] (2026-06-06T21:52Z) Recorded validation evidence and final outcome.

## Surprises & Discoveries

- Observation: Running `make` inside an ad hoc Nix shell can still fail if old in-repo tool binaries are present from a non-Nix environment.
  Evidence: `tools/gbafix/gbafix` failed at the final ELF fix step with `Could not start dynamically linked executable`, even though the ARM link step had succeeded.

- Observation: Plain `nix build .#default` in a temporary non-Git directory is not a valid simulation of a clean checkout for this Nix version.
  Evidence: Nix failed before building while fetching `git+file:///tmp`. Repeating the simulation from a temporary Git repository with the copied files committed made plain `nix build` work.

- Observation: The existing test runner binary is not NixOS-runnable as packaged in this repository.
  Evidence: `nix develop path:$PWD -c tools/mgba/mgba-rom-test --help` failed with `Could not start dynamically linked executable: tools/mgba/mgba-rom-test` and pointed to the NixOS stub loader guidance.

## Decision Log

- Decision: Use `pkgs.gcc-arm-embedded` instead of `pkgsCross.arm-embedded.stdenv.cc`.
  Rationale: The repository's `Makefile` expects `arm-none-eabi-*` tools on PATH and uses `arm-none-eabi-gcc -mthumb -print-file-name=...` to locate newlib and libgcc. The `gcc-arm-embedded` package provides the expected command names and libraries directly.
  Date/Author: 2026-06-06 / Codex

- Decision: Touch `.histignore` inside the derivation build directory rather than editing `check_history.sh`.
  Rationale: `check_history.sh` already documents `.histignore` as the escape hatch for builds without Git history, and the repository Dockerfile already uses the same approach for container builds.
  Date/Author: 2026-06-06 / Codex

- Decision: Run `make clean-tools clean-check-tools` before the Nix build.
  Rationale: Clean source checkouts should not contain ignored tool binaries, but cleaning first makes the derivation robust when evaluated from a local path that may include stale host-built tools.
  Date/Author: 2026-06-06 / Codex

- Decision: Use `src = ./.` for the derivation source.
  Rationale: `./.` directly references the flake source directory and avoids coupling the derivation source to the flake output object. Plain `nix build` should be verified from a Git checkout, because this Nix version resolves non-Git `.` installables through Git-flake logic.
  Date/Author: 2026-06-06 / Codex

- Decision: Keep `make check` out of this flake deliverable.
  Rationale: The requested deliverable is a ROM build derivation. The project test command requires an mGBA ROM-test executable, and the tracked `tools/mgba/mgba-rom-test` binary cannot run on NixOS without separate packaging or patching.
  Date/Author: 2026-06-06 / Codex

## Outcomes & Retrospective

The flake is implemented and verified for the requested ROM build. `nix build` succeeds from a temporary committed Git checkout made from the current tracked files plus `flake.nix`, `flake.lock`, and this plan, producing `pokeemerald.gba`, `pokeemerald.elf`, and `pokeemerald.map` in the Nix output. The development shell also exposes the Nix-provided ARM toolchain through `DEVKITARM`.

`make check` was not added as a Nix package target because the existing mGBA ROM-test binary in `tools/mgba/` is a generic dynamically linked executable that does not run on NixOS. Packaging or replacing that test runner remains a separate task.

## Context and Orientation

This repository builds a Game Boy Advance ROM with a GNU Make build system. The root `Makefile` builds native helper tools under `tools/`, generates map and data files, compiles C and assembly with `arm-none-eabi-*` programs, links `pokeemerald.elf`, and converts it to `pokeemerald.gba`. `DEVKITARM` is optional in the Makefile: if it points to a directory with `bin/`, that directory is prepended to `PATH`; otherwise the build uses `arm-none-eabi-*` tools already on `PATH`.

The Dockerfile documents the minimal external packages used by the project: native build tools, Python 3, pkg-config, libpng, zlib, Perl, Git, `gcc-arm-none-eabi`, `binutils-arm-none-eabi`, and newlib. The Nix flake mirrors those requirements with `gcc-arm-embedded`, `gcc`, `gnumake`, `pkg-config`, `libpng`, `zlib`, `python3`, `perl`, and `git`.

The repository's `make_tools.mk` runs `check_history.sh` before building tools. A Nix derivation builds from a source copy that does not have `.git` history, so the build creates `.histignore` in the temporary build directory before invoking `make`.

## Plan of Work

Add `flake.nix` at the repository root. It should expose `packages.<system>.default` and `packages.<system>.pokeemerald`, both backed by one derivation. The derivation should use `src = ./.`, clean in-repo tools, create `.histignore`, run `make -j"$NIX_BUILD_CORES" NO_MULTIBOOT=1`, and install `pokeemerald.gba`, `pokeemerald.elf`, and `pokeemerald.map` into `$out`.

Expose a default development shell with the same native tools and ARM compiler so developers can run `nix develop` and then use normal Makefile commands. Update `docs/install/linux/NIXOS.md` to prefer `nix build` for reproducible builds and `nix develop` for interactive work.

Generate `flake.lock` with the pinned nixpkgs revision. Then run `nix build` from the repository root and inspect `result/` for the ROM output. If the build fails, adjust only the Nix files or narrowly necessary build-environment compatibility settings.

## Concrete Steps

From `/home/bayesartre/dev/pokeemerald-expansion-shared-power`, run:

    nix flake lock
    nix build
    ls -l result

Expected successful output includes a `result` symlink to a Nix store path and these files:

    pokeemerald.gba
    pokeemerald.elf
    pokeemerald.map

For interactive development, run:

    nix develop
    make clean-tools
    make -j"$(nproc)" NO_MULTIBOOT=1

The `make clean-tools` step is useful when the working tree already contains native helper binaries built outside Nix.

Local verification before committing new flake files used two equivalent forms. While the new files are untracked in the active repository, `nix build path:$PWD` makes them visible to Nix. To prove plain `nix build`, create a temporary Git checkout containing the tracked files and new flake files, commit them in that temporary directory, and run `nix build` there. That second verification matches how a clean checkout behaves after these files are tracked.

## Validation and Acceptance

Acceptance for this plan is a successful `nix build` at the repository root and a Nix output containing `pokeemerald.gba`. The build must use the derivation-provided ARM compiler and native libraries rather than host `DEVKITARM`. Evidence should include the exact `nix build` command, the final build status, and a listing of the output files.

`make check` is not accepted as part of this flake change because its mGBA runner is not NixOS-runnable in the current repository. A future task can package a Nix-native mGBA ROM-test runner or replace the binary dependency.

## Idempotence and Recovery

The Nix build happens in a temporary build directory and does not modify the working tree. Re-running `nix build` should reuse cached inputs where possible and rebuild deterministically when source files change. If a local interactive build fails because stale helper binaries were built outside Nix, run `nix develop -c make clean-tools clean-check-tools` and retry.

## Artifacts and Notes

Ad hoc Nix shell probe before adding the flake:

    nix shell --impure nixpkgs#gcc-arm-embedded nixpkgs#gnumake nixpkgs#gcc nixpkgs#pkg-config nixpkgs#libpng nixpkgs#zlib nixpkgs#python3 nixpkgs#perl nixpkgs#git -c make -j"$(nproc)" NO_MULTIBOOT=1
    arm-none-eabi-ld: warning: ../../pokeemerald.elf has a LOAD segment with RWX permissions
    tools/gbafix/gbafix pokeemerald.elf -t"POKEMON EMER" -cBPEE -m01 -r0 --silent
    Could not start dynamically linked executable: tools/gbafix/gbafix

This proves the package set can reach the link stage and that stale helper binaries are the immediate NixOS-specific failure mode for interactive builds.

Pinned nixpkgs input:

    nix flake lock path:$PWD
    Added input 'nixpkgs':
      github:NixOS/nixpkgs/331800de5053fcebacf6813adb5db9c9dca22a0c

Local path flake build:

    nix build path:$PWD
    result -> /nix/store/yfg3yhfrd8pcz5g572hwsp5f1y0p405f-pokeemerald-expansion-shared-power-0-unstable
    pokeemerald.elf 33122416 bytes
    pokeemerald.gba 33554432 bytes
    pokeemerald.map 3748190 bytes

Temporary committed checkout build:

    nix build
    result -> /nix/store/cdi5ni3dvra8w1v0j112wwgk47zp2vh7-pokeemerald-expansion-shared-power-0-unstable
    pokeemerald.elf 33095968 bytes
    pokeemerald.gba 33554432 bytes
    pokeemerald.map 3748655 bytes

Development shell probe:

    nix develop path:$PWD -c sh -lc 'printf "DEVKITARM=%s\n" "$DEVKITARM"; command -v arm-none-eabi-gcc; arm-none-eabi-gcc --version | sed -n 1p; pkg-config --modversion libpng'
    DEVKITARM=/nix/store/mmkh2v78liwvll9ikdamv3iqwy5drm1g-gcc-arm-embedded-15.2.rel1
    /nix/store/mmkh2v78liwvll9ikdamv3iqwy5drm1g-gcc-arm-embedded-15.2.rel1/bin/arm-none-eabi-gcc
    arm-none-eabi-gcc (Arm GNU Toolchain 15.2.Rel1 (Build arm-15.86)) 15.2.1 20251203
    1.6.56

Test-runner feasibility probe:

    nix develop path:$PWD -c tools/mgba/mgba-rom-test --help
    Could not start dynamically linked executable: tools/mgba/mgba-rom-test

## Interfaces and Dependencies

The flake depends on `nixpkgs` and uses `pkgs.stdenv.mkDerivation`. The derivation's build interface is the existing root `Makefile`, not a new build script. The derivation source is `./.`, meaning the flake directory itself. The key environment variable is `DEVKITARM`, which is set to the Nix store path for `pkgs.gcc-arm-embedded` so the Makefile can place its `bin/` directory on PATH.

Revision Note (2026-06-06): Initial plan created for adding and verifying a Nix flake derivation that builds the ROM from source.

Revision Note (2026-06-06): Switched the derivation source from `self` to `./.` after temporary path verification showed source resolution ambiguity.

Revision Note (2026-06-06): Added final validation evidence, the temporary committed-checkout proof for plain `nix build`, and the `make check` caveat.
