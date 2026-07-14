# Promote Nix flake checks to the full test suite

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This document is maintained according to `.agent/PLANS.md`.

## Purpose / Big Picture

The Nix flake currently proves that the ROM builds and that the mGBA test-runner path works with a focused `uq4_12` smoke. After this work, Nix should be the complete validation path: the full unfiltered repository `make check` suite should pass through the Nix-provided mGBA runner, and `nix flake check` should run that full suite instead of the smoke target.

## Progress

- [x] (2026-06-06T23:30Z) Confirmed the prior Nix commit is `57ee254cbe Add Nix ROM build and test checks`.
- [x] (2026-06-06T23:30Z) Confirmed the current dirty worktree has unrelated tracker/gameplay changes, so this task must keep edits scoped.
- [x] (2026-06-06T23:40Z) Cleared generated test/tool artifacts and reproduced that the full Nix dev-shell check currently fails before the test runner because host-side tools were linked for generic `/lib64` execution.
- [x] (2026-06-06T23:40Z) Updated `flake.nix` to provide explicit host-tool linker flags and Linux runtime library variables in the Nix dev shell and build derivations.
- [x] (2026-06-07T00:05Z) Reproduced the current full-suite Nix failures from the actual worktree: six failed tests and three stale assumption failures after the test runner completed.
- [x] (2026-06-07T00:30Z) Diagnosed and fixed the focused Drought, Limber, Protosynthesis popup, Booster Energy, Micle Berry, AI accuracy, and smart-switching failures without narrowing the test suite.
- [x] (2026-06-07T00:30Z) Validated the focused fixes under Nix with targeted `make NO_MULTIBOOT=1 TESTS="..." check` commands.
- [x] (2026-06-07T01:04Z) Verified full `nix develop -c make NO_MULTIBOOT=1 check` passes with 3324 passed, 17 known-failing, 787 todo, and 4128 total tests.
- [x] (2026-06-07T01:05Z) Promoted `flake.nix` from `TESTS="uq4_12"` smoke to full `make check`, and updated the NixOS install doc wording to match.
- [x] (2026-06-07T01:35Z) Reproduced an artifact-free flake-only failure in `Condition Coach rejects Eggs`, caused by that test depending on leftover badge state from prior tests.
- [x] (2026-06-07T01:55Z) Fixed and validated the isolated `Condition Coach rejects Eggs` test under Nix after force-rebuilding stale local host tools with the Nix interpreter.
- [x] (2026-06-07T02:05Z) Verified an artifact-free temporary checkout at `/tmp/tmp.R67kaivo5X/repo` passes `nix flake check`.
- [x] (2026-06-07T02:05Z) Recorded final flake evidence, derivation outputs, and documentation decisions.

## Surprises & Discoveries

- A dirty worktree can contain ignored host-built tool binaries such as `tools/scaninc/scaninc` and `tools/preproc/preproc`. Running the full Nix check without clearing them can fail in dependency scanning before any tests execute.
- Rebuilding those tools inside the original Nix dev shell still produced ELF binaries requesting `/lib64/ld-linux-x86-64.so.2` and lacking a runtime path for `libstdc++.so.6`, so the shell needs explicit linker/runtime settings for repo-built host tools.
- The Nix host-tool issue can reappear for individual tools that were not rebuilt by an earlier focused target. Running `nix develop -c make clean-tools clean-check-tools tools check-tools` restored an authoritative Nix-built tool set before focused testing.
- This repo's current Gen 6+ Drought behavior is permanent entry weather, so Heat Rock does not extend it and tests must not expect "The sunlight faded." from Drought alone.
- A Protosynthesis holder with Booster Energy that starts on the field without sun consumes the item before it can create finite sun. To test item activation after finite sun expires, the holder must switch in while finite Sunny Day is already active.
- The smart-switching failure was not a switch-target regression. With the random wrapper removed temporarily, the concrete mismatch was that Rhyhorn now scores Rock Tomb equal to Bulldoze and chooses Rock Tomb before the expected Gligar send-out.
- The artifact-free derivation runs a clean test build and exposed extra Condition Coach coverage that a stale working-tree check missed: `Condition Coach rejects Eggs` returned `CONDITION_COACH_RESULT_LOCKED` before the egg guard because it did not initialize badge state.
- Local focused validation can still be blocked by stale host tools even after the flake is fixed; `readelf` confirmed old `preproc` and `gbafix` binaries requested `/lib64/ld-linux-x86-64.so.2`, and force-rebuilding through `make_tools.mk` restored the Nix glibc interpreter.

## Decision Log

- Decision: Treat the existing full-suite failures as behavior/test issues to investigate, not as Nix infrastructure failures.
  Rationale: The previous Nix work proved that the mGBA runner starts and executes tests; remaining failures must be addressed at their mechanics or expectation source.
  Date/Author: 2026-06-06 / Codex
- Decision: Make the Nix dev shell and derivations pass explicit linker/runtime settings to host-side tools.
  Rationale: Full-suite validation depends on rebuilding and executing in-repo C/C++ tools before the test ROM runs; those tools must not depend on host `/lib64` behavior on NixOS.
  Date/Author: 2026-06-06 / Codex
- Decision: Update Drought tests to assert permanent entry sun rather than finite Gen 6+ sun.
  Rationale: The engine currently treats ability-created entry weather as permanent in this repo; the stale test expected upstream finite-weather behavior and contradicted observed intended mechanics.
  Date/Author: 2026-06-07 / Codex
- Decision: Fix the Limber paralysis path so Thunder Wave's status-move type-effectiveness check runs before Limber blocks paralysis.
  Rationale: Electric-type immunity and no-effect status moves should still take precedence, but Limber should produce its ability-prevention script once the move can otherwise paralyze.
  Date/Author: 2026-06-07 / Codex
- Decision: Rewrite the Booster Energy finite-sun fixture to switch Raging Bolt in under opponent-created Sunny Day.
  Rationale: Starting the holder on the field without sun tests immediate Booster Energy consumption, not activation after finite sun expires.
  Date/Author: 2026-06-07 / Codex
- Decision: Keep the smart-switching test's switch expectation but update Rhyhorn's first expected move to Rock Tomb.
  Rationale: A diagnostic run showed Rock Tomb and Bulldoze tie at the best AI score; Rock Tomb is selected, and the Gligar send-out and follow-up behavior pass once the stale move expectation is corrected.
  Date/Author: 2026-06-07 / Codex
- Decision: Promote the flake test package itself instead of leaving a smoke-named package that happens to run the full suite.
  Rationale: `checks.<system>.test` should be self-describing after this work; a `pokeemerald-test` package with a full-suite description is clearer than `pokeemerald-test-smoke` running non-smoke validation.
  Date/Author: 2026-06-07 / Codex
- Decision: Fix the Condition Coach egg test by making its prerequisites explicit instead of changing Condition Coach behavior.
  Rationale: The implementation intentionally checks unlock requirements before mon eligibility; the egg test should set full badges when it wants to isolate the egg rejection path.
  Date/Author: 2026-06-07 / Codex

## Outcomes & Retrospective

Focused fixes are implemented and individually validated under Nix, the full unfiltered Nix `make check` passes in the working tree, and the flake test check now runs full `make check`. A first artifact-free `nix flake check` found one additional test-isolation issue in Condition Coach; after the setup fix, the artifact-free rerun passed with the full suite. No additional gameplay change-list entry was added: the final Condition Coach change is test setup only, and the earlier Limber change restores the intended ability-prevention path covered by battle tests rather than introducing a new player-facing rule.

## Context and Orientation

The root `flake.nix` defines `packages.<system>.pokeemerald-test-smoke`, which currently builds the test ELF and runs `make -j"$NIX_BUILD_CORES" NO_MULTIBOOT=1 TESTS="uq4_12" check`. That proves the Nix mGBA runner path but not the full repository suite. The root `Makefile` runs full tests with `make check`; under Nix, `mgba-rom-test` and `mgba-rom-test-hydra` are supplied by the flake dev shell and test derivation.

Previous evidence showed the full suite could run under Nix but failed in the dirty tree with seven behavioral failures around Condition Coach, Limber/Thunder Wave, Drought and Heat Rock weather messages, Booster Energy/Protosynthesis, AI smart switching, and Protosynthesis popup animation. The first step is to rerun the suite against the current worktree because unrelated pending changes may have shifted the failure list.

## Plan of Work

Run the full unfiltered suite through the Nix dev shell and capture the current failures. For each failure, inspect the relevant test and implementation paths, then make the narrowest behavior or expectation fix that preserves intended mechanics. If a fix changes player-visible battle or gameplay rules, update `docs/gameplay_changes_public.md` or `docs/gameplay_changes_blind.md` according to the spoiler policy. After the full suite is green, update `flake.nix` so `checks.<system>.test` runs the full suite rather than the smoke. Finish with an artifact-free temporary Git checkout running `nix flake check`.

## Concrete Steps

From `/home/bayesartre/dev/pokeemerald-expansion-shared-power`, run:

    nix develop -c make NO_MULTIBOOT=1 check

After fixes, update `flake.nix` so the Nix test check no longer passes `TESTS="uq4_12"` to `make check`. Then verify with:

    nix develop -c make NO_MULTIBOOT=1 check
    nix flake check

For artifact-free proof, copy tracked files plus any untracked plan/flake files into a temporary Git repository, commit them there, and run `nix flake check`.

## Validation and Acceptance

Acceptance requires a full unfiltered `nix develop -c make NO_MULTIBOOT=1 check` pass in the working tree and a green artifact-free `nix flake check` where `checks.<system>.test` runs full `make check`. The final evidence must include the full test summary and the flake check result.

## Idempotence and Recovery

Nix derivations build in temporary directories. Interactive `nix develop -c make ...` commands can leave ignored build outputs in the workspace; remove generated ROM, ELF, map, and `result` artifacts before final handoff. If stale objects or tools confuse local results, run `nix develop -c make tidycheck clean-tools clean-check-tools` and rerun the suite.

## Artifacts and Notes

Focused validation evidence so far:

    nix develop -c make NO_MULTIBOOT=1 TESTS="Booster Energy will activate Protosynthesis after harsh sunlight ends" check
    Result: PASS, 1 total.

    nix develop -c make NO_MULTIBOOT=1 TESTS="Micle Berry increases" check
    Result: PASS, 2 total.

    nix develop -c make NO_MULTIBOOT=1 TESTS="AI prefers moves with better accuracy" check
    Result: PASS, 1 total.

    nix develop -c make NO_MULTIBOOT=1 TESTS="AI_FLAG_SMART_SWITCHING: AI won't send out defensive mon" check
    Result: PASS, 1 total.

    nix develop -c make -f make_tools.mk clean-tools tools
    Result: PASS; key host tools requested the Nix glibc interpreter.

    nix develop -c make NO_MULTIBOOT=1 TESTS="Condition Coach rejects Eggs" check
    Result: PASS, 1 total.

Full-suite validation evidence:

    nix develop -c make NO_MULTIBOOT=1 check
    Result: PASS.
    Summary: KNOWN_FAILING 17, TO_DO 787, PASSED 3324, TOTAL 4128.

Artifact-free flake validation evidence:

    /tmp/tmp.R67kaivo5X/repo
    nix flake check
    Result: PASS.
    Checks built: checks.x86_64-linux.rom-build and checks.x86_64-linux.test.
    ROM output: /nix/store/xjkahxn0jzvlfvb2s2vyk06d8sjdadlm-pokeemerald-expansion-shared-power-0-unstable
    Test output: /nix/store/w5h167vqg5z16ipz6ip6f2h21zrd1n9x-pokeemerald-expansion-shared-power-test-0-unstable
    Test summary: KNOWN_FAILING 18, TO_DO 792, PASSED 3359, TOTAL 4169.
    Test derivation buildPhase completed in 6 minutes 53 seconds.

## Interfaces and Dependencies

Primary interfaces are the existing `Makefile` `check` target, the in-repo C test framework under `test/` and `include/test/`, and `flake.nix` checks. Avoid changing Docker or direct Makefile workflows except for narrowly necessary bug fixes.

Revision Note (2026-06-06): Initial plan created for promoting Nix flake checks from a test-runner smoke to the full repository test suite.

Revision Note (2026-06-07): Recorded the full-suite failure list, focused test fixes, host-tool rebuild recovery, and targeted Nix validation evidence before rerunning the full suite.

Revision Note (2026-06-07): Recorded the full unfiltered Nix test-suite pass and the flake promotion from smoke test to full `make check`.

Revision Note (2026-06-07): Recorded the clean flake derivation's Condition Coach test-isolation failure and the narrow test setup fix.

Revision Note (2026-06-07): Recorded focused Condition Coach validation and the local host-tool interpreter rebuild needed for Nix shell testing.

Revision Note (2026-06-07): Recorded the successful artifact-free full-suite `nix flake check` and final output paths.
