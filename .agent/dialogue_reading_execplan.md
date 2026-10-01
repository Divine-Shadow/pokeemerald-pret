# Extend consistent dialogue reading controls

This living plan follows `.agent/PLANS.md`. Update Progress, Surprises & Discoveries, Decision Log, and Outcomes & Retrospective during execution.

## Purpose / Big Picture

Players can press A to reveal the current pane and hold R to read faster in incoming/scripted PokéNav calls, calls inside the PokéNav menu, and battle dialogue. The same A press cannot reveal and hang up or select a choice. Battle messages keep automatic timeouts. Sound effects, cries, animations, window transitions, and meaningful embedded text pauses finish normally. Link and recorded battles retain existing timing.

## Progress

- [x] Verified exact agent1 checkout, clean base e391055e3d266999ab46f775c4650cc1371cfc46, repository instructions and Nix route.
- [x] Apply consistent reading controls to both real call printers and battle dialogue; narrow 753 candidate delays to 695 audited reading waits.
- [x] Fix the reviewed R release/repress continuation gap; verify uninterrupted holding, expiry and ineligibility regressions.
- [x] Apply the separately authorized baseline Ability Swapper nickname correction and both refusal regressions.
- [x] Reproduce and correct test-fixture sound isolation without changing runtime sound gates.
- [x] Pass final focused, complete local, normal/LTO ROM, and clean-source Nix gates; record evidence below.
- [x] Review the final diff and commit the nickname correction separately from the reading implementation, preserving existing history.
- [ ] End-to-end interactive validation is blocked by the supported runner/access errors below; no live save or foreground session was used.

## Surprises & Discoveries

The Desktop logical path `/workspaces/dev/pokeemerald-expansion-shared-power` maps to the host checkout `/home/bayesartre/dev/project-worktrees/pokeemerald-agent1/pokeemerald-expansion-shared-power`. The literal host `/workspaces/...` path is a distinct main checkout and must not be used.

The shared printer already supports bursts, but only the ordinary field task enabled them. Calls allocate their own window IDs. Battle messages use window zero; their string controller and reading-delay command are separate.

Ability popups can stay active until a message wait returns and a later script command destroys them. Requiring all popups to disappear inside a reading wait would deadlock. Preserve explicit `waitabilitypopup` gates instead.

The first focused run exposed reuse of A when VBlank interrupts occurred during burst rendering. Input ownership now uses gInputFrameCounter, incremented only by ReadKeys, while dwell and transition deadlines still use elapsed VBlank time. The same sampled edge remains consumed across rendering interrupts.

The Nix development shell omits which, required by check_history.sh. Append the already-installed /run/current-system/sw/bin to this command's PATH after the Nix tools. Do not create .histignore in the user checkout or modify the shell configuration.

The full suite's CRASH entry is deliberate harness coverage: test/test_test_runner.c marks Tests resume after CRASH with KNOWN_CRASHING and calls a null function. The runner accepts its expected result and exits successfully.

The documented Scala emulator route cannot currently run: Docker reports that /workspaces/dev/.devkit/native-broker/broker.sock is unreachable, and the named external runner's worktree Git metadata points to a missing worktree entry. No executable exists at /usr/local/bin/mgba-headless, /run/current-system/sw/bin/mgba, or /run/current-system/sw/bin/mgba-headless. No repair, substitute source checkout, save modification, or foreground takeover was performed.

Review exposed an R-release gap: FinishTextReading disables the active owner but retains brief continuation. UpdateTextReading must process release and ineligibility before returning for a disabled owner. The exact release-one-frame/repress-within-four-VBlanks regression now passes, alongside uninterrupted continuation and expiry/ineligibility cases.

The separately reported /161 Ability Swapper refusal is a baseline string-buffer defect: GetSelection returned before copying the chosen nickname, leaving party-menu maximum HP in gStringVar1. The stored nickname is intact. Move the existing copy after Egg refusal and before ability eligibility checks; do not change eligibility or payment. Tests seed /161 and check both preview/swap refusals with custom-named Mudkip, unchanged stored nickname, ability, and Heart Scale quantity.

## Decision Log

The first final full-suite and clean-source runs both failed only the timeout regression's ready assertion (test/text_reading_test.c:352). Focused tests passed in a fresh sound environment. Starting a real SE track before setup reproduced the same assertion in the focused suite. The guarded wait correctly retained the sound gate; the fixture failed to establish its assumed idle presentation. Test setup now stops both SE players and any initialized cry, and clears animation state. Keep the active-SE precondition so isolation is verified. Production reading logic and sound guards receive no change from this correction. Rerun focused and complete gates on this final fixture.

On 2026-09-30, use a separate TextReadingState for each call context and battle dialogue. Use a fresh A edge only for the active pane; guard final confirmation against an edge used by the printer. R qualifies after ten eligible frames and waits six frames after presentation finishes before final continuation.

On 2026-09-30, preserve all embedded timed pauses in call and battle strings. Existing field pause behavior remains unchanged. Add an explicit `waitreadmessage` command only for standard long reading durations in the same basic block immediately following printstring, printfromtable, attackstring, critmessage, or resultmessage, excluding empty strings. Leave literal frame counts, popup-only/capture/form-change/link waits, and every pause command unchanged.

## Outcomes & Retrospective

The implemented reading state is attached to both actual call-printer helpers and battle narrative printers. After narrowing 753 lexical candidates, 695 long same-basic-block reading waits use the new native command; 58 additional candidates and all other synchronization/pacing waits remain fixed. Link/recorded/headless execution uses the legacy waitmessage counter algorithm. No save format or runtime dependency changed.

Final gates pass: reading 14/14, printers 11/11, Ability Swapper 11/11, full make check (3,748 passed, 19 known failing, 778 TODO, 4,545 total, exit 0), normal ROM, isolated LTO ROM with UNUSED_ERROR=1, and clean-source nix flake check. The earlier full-suite/clean-source failure and its focused stale-SE reproduction are corrected by test setup isolation, as recorded in Decision Log. The CRASH harness case remains expected. No unexpected test failure remains in the final gates.

Interactive limitation: the actual call printers and native battle wait execute under mGBA ROM tests with injected input states. SE and WAIT_SE fixtures start actual music-player tracks; the cry fixture invokes the actual predicate through a deterministic active/inactive/null channel fixture. These tests do not certify end-to-end incoming-call/menu scenes, on-screen animation overlap, perceived reading speed, or audio playback. The supported interactive runner is unavailable as described above.

## Context and Orientation

`src/text.c` renders text and handles page, sound, and embedded-pause states. `src/text_reading.c` handles input ownership, R qualification, and final reading dwell. `src/match_call.c` owns incoming/scripted calls; `src/pokenav_match_call_gfx.c` and `src/pokenav_match_call_list.c` own menu calls and final exit. `src/battle_presentation.c`, `src/battle_message.c`, and `src/battle_main.c` attach reading controls to narrative printers. Controller/menu and generic delay boundaries reset the reading state. `BS_WaitMessageOrButton` in `src/battle_script_commands.c` accepts eligible reading input only after controller, printer, animation, cry, and SE gates, while preserving its timeout. The script audit is in `docs/engineering/reading_wait_audit.md`.

## Plan of Work

Milestone one is the audited source change plus focused tests. Compile and run the actual incoming and menu call printer fixtures, A/page/final confirmation cases, R qualification and post-presentation dwell, resets, timed pauses, and multiplayer/headless exclusions. Resolve compile/test failures before milestone two.

Milestone two is full validation. Use the repository Nix shell to run the complete tests and build the ROM, then run `nix flake check` for clean-source ROM/test derivations. The CI workflow also lists an LTO ROM build. Inspect an already-supported interactive emulator surface if available; do not overwrite user saves or claim that headless battle tests prove UI behavior.

Milestone three is review and custody. Run Git whitespace and status checks, preserve unrelated work, update this plan and patch notes with exact evidence, and commit the scoped implementation. Do not push/merge/deploy or rewrite history.

## Concrete Steps

All repository operations use the configured `codex-gui-pokeemerald-agent1` route and the declared host checkout above. Resolve `nix` from `/run/current-system/sw/bin/nix`; the host SSH shell does not supply development tools in PATH.

    nix develop -c make NO_MULTIBOOT=1 TESTS="*reading" check
    nix develop -c make -j4 NO_MULTIBOOT=1 check
    nix flake check

The runner accepts one case-sensitive prefix or a leading-asterisk infix, not a pipe-separated expression. Capture noisy output through `dev_scripts/run_logged.sh` under ignored `build_artifacts/logs/`.

## Validation and Acceptance

The real call runners render on a dynamically allocated test window, A completes only the current pane, a later fresh A confirms, and R accelerates then dwells after sound/presentation readiness. New call/battle printer tests preserve timed pauses, and existing field printer tests remain unchanged. Reading commands must not bypass animation/controller/cry/SE gates or explicit ability-popup waits. Menu/scene resets discard continuation. Full tests and ROM builds must pass. Interactive verification, if unavailable, remains an explicit limit in the report.

## Idempotence and Recovery

Before applying source changes, require the exact base revision and matching hashes for every preimage. Later edits require matching current preimages and must not overwrite concurrent source work. Builds create only the project's normal ignored outputs. Do not clean user saves, copy credentials, repair fleet infrastructure, reset the branch, or force push. Preserve the reviewable patch and evidence if a canonical validation route is blocked.

## Artifacts and Notes

Base: e391055e3d266999ab46f775c4650cc1371cfc46, branch agent1, initially clean and ahead of origin/main by one existing commit.

All make commands below run under nix develop with PATH="$PATH:/run/current-system/sw/bin", LOG_SILENT=1, and bash dev_scripts/run_logged.sh. Logs are relative to the exact agent1 checkout.

| Gate | Command passed to run_logged.sh | Evidence |
| --- | --- | --- |
| Reading, final fixture | make -j4 NO_MULTIBOOT=1 TESTS="*reading" check | build_artifacts/logs/20260930_194457_dialogue-reading-focused.log, 14/14, exit 0 |
| Printers, final source | make -j4 NO_MULTIBOOT=1 TESTS="Dialogue text printer" check | build_artifacts/logs/20260930_194700_dialogue-printers.log, 11/11, exit 0 |
| Ability Swapper | make -j4 NO_MULTIBOOT=1 TESTS="Ability Swapper" check | build_artifacts/logs/20260930_192747_ability-nickname.log, 11/11, exit 0 |
| Full local suite | make -j4 NO_MULTIBOOT=1 check | build_artifacts/logs/20260930_194826_dialogue-full.log, 3,748 passed, 19 known failing, 778 TODO, 4,545 total, exit 0 |
| Normal ROM | make -j4 NO_MULTIBOOT=1 | build_artifacts/logs/20260930_193340_dialogue-rom.log, final runtime source, exit 0 |
| LTO and unused warnings | make -j4 NO_MULTIBOOT=1 LTO=1 UNUSED_ERROR=1 OBJ_DIR_NAME=build/dialogue-lto FILE_NAME=build_artifacts/dialogue-lto/pokeemerald | build_artifacts/logs/20260930_193347_dialogue-lto.log, final runtime source, exit 0 |
| Clean-source ROM and full suite | nix flake check --max-jobs 1 --cores 4 | build_artifacts/logs/20260930_194652_dialogue-flake.log, x86_64-linux checks; incompatible cross-host systems not run, exit 0 |

LTO emits existing linker/newlib and mystery_gift_client warnings; it completed successfully. No unrelated warning fixes were included.

## Interfaces and Dependencies

TextReadingState and its begin/update/advance/reset/finish APIs are declared in `include/text_reading.h`. Existing text printer APIs gain SetTextPrinterPreserveTimedPauses. Battle wrappers are declared in `include/battle_presentation.h`. Test-only call entry points invoke the same actual initialization/run helpers used by each call path. No new runtime dependency or save format is introduced.

Revision note (2026-09-30): Record applied source, the narrower wait audit, the input ownership race and fix, completed validation evidence, and exact interactive-access limitations. Clean-source checks and final custody remain pending.

Revision note (2026-09-30): Address the independent continuation review finding and record the separately authorized baseline nickname correction. Revalidate the final combined source; do not treat earlier gates as final-source results.

Revision note (2026-09-30): Record every completed final gate, the reproduced fixture failure and correction, and the two-commit custody boundary. Interactive behavior and human audio playback remain unverified; source and tests have not changed after these gates.
