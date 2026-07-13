---
name: pokeemerald-gameplay-updater
description: Update gameplay content in this pokeemerald-expansion repo, including Pokemon, moves, abilities, learnsets, evolutions, trainers, maps, items, encounters, dialogue, events, and related tests, while maintaining spoiler-aware public and blind-friendly player change lists.
---

# Pokeemerald Gameplay Updater

Use this skill for gameplay-content work in this repository. Keep technical implementation, public player knowledge, and blind-playthrough discovery notes separated.

## Core workflow

1. Load [references/gameplay-file-map.md](references/gameplay-file-map.md) before editing unfamiliar gameplay areas.
2. Classify each player-visible change before or while implementing it:
   - Public: players should know before planning a run.
   - Blind-friendly: players should discover naturally during play.
   - Mixed: split the public rule or mechanic from the spoiler-sensitive placement, team, script, or reveal.
3. Search for existing player-facing gameplay note files before creating new ones:
   `rg -n "Public Gameplay|Blind|Spoiler|gameplay changes|player-facing" docs .codex PATCH_NOTES.md`
4. If no better repo convention exists, maintain these files:
   - `docs/gameplay_changes_public.md`
   - `docs/gameplay_changes_blind.md`
5. Keep `PATCH_NOTES.md` current at the top of the file after every code, data, or docs change, using the repository's required `- Area: short description (commit <short-hash>).` format.
6. Run the narrowest useful validation first, then broader validation when shared mechanics, generated data, or battle behavior changed. Prefer `make check` for risky battle/gameplay logic and `make` for generated data or ROM integration.

## NPC and map-object placement

When adding a new NPC, default to making them a named NPC with a role-specific local id, script label, and dialogue labels, and create/use a new role-specific sprite instead of reusing a stock sprite. The exception is when the task is explicitly about an already-existing named NPC; in that case, preserve that NPC's identity and sprite unless the user asks for a sprite change.

When adding, moving, or removing checkable NPC object events in `data/maps/*/map.json`, run the bundled static accessibility checker on each touched map before final validation:

```bash
python3 .codex/skills/pokeemerald-gameplay-updater/scripts/check_npc_access.py data/maps/<MapName>/map.json
```

Use `--changed` after a batch of map edits to scan changed map JSON files. Use `--include-flagged-targets` when a new NPC is hidden behind an event flag but still needs to be reachable whenever visible. The checker uses layout `map.bin` collision, metatile counter behavior, warps, and map connections to verify that a land player can reach an adjacent or counter-facing talk tile. It reports maps with no static warp or connection seed as `UNSUPPORTED`; use emulator/manual proof for puzzle-state, surf-only, bike-only, story-gated, scripted movement, or unsupported access. Treat `--all --report-only` as an audit to find suspicious placements, not as a clean repo-wide gate.

## Spoiler policy

Treat these as public unless the user explicitly says otherwise:

- Pokemon stat, type, ability-slot, form, learnset, tutor, TM/HM, egg-move, or evolution-method changes.
- Move type, category, power, accuracy, PP, priority, flags, effect, targeting, or animation-relevant behavior changes.
- Ability list membership, ability mechanics, item mechanics, global config, rule toggles, and battle-system behavior.
- Broad availability rules that affect teambuilding before a run.

Treat these as blind-friendly by default:

- Trainer teams, gym sets, Elite Four/champion sets, rival teams, battle rematches, trainer AI surprises, and trainer party pools.
- Added, removed, or moved trainers and NPCs.
- Item-ball, hidden-item, gift-item, shop-inventory, reward, and pickup-location changes, including TM/HM locations.
- Map scripts, story events, optional events, puzzle states, signposts, dialogue branches, and new dialogue.
- Exact wild encounter placements when the change is meant to reward exploration rather than pre-run planning.

For ambiguous changes, default to blind-friendly if the information answers "where is it or who has it?" Default to public if it answers "what are the rules or what can this Pokemon/move/item do?"

## Player-note writing

Write player notes in player terms, not implementation terms. Prefer "Strength is now a Rock-type move with 80 power" over constants or filenames.

Public entries should be concise and useful before a run. Avoid exact trainer rosters, item coordinates, event flags, and surprise dialogue in public notes.

Spoiler-free notes must name the changed rule or availability directly. Avoid vague summaries like "Some moves have had type changes." Do not guess old or unchanged values; verify them or omit that column.

Move changes are always public and explicit. Prefer a table when multiple move fields are relevant:

| Move | New type | New accuracy | New power |
| --- | --- | --- | --- |
| Strength | Rock | 100 (unchanged) | 80 (unchanged) |

Pokemon move-access, learnset, tutor, TM/HM, and egg-move changes must also be explicit in public notes. This says what is available, not who uses it in a trainer battle:

| Pokemon | Move | Learns at |
| --- | --- | --- |
| Metang | Swords Dance | Level 57 |
| Ninjask | Hurricane | Tutor |

Trainer sets stay blind-friendly: public notes must not reveal which trainer has which Pokemon or moves. For example, do not publicly write "Roxanne's Geodude is now a Hippowdon" or "Liza's Metagross has Swords Dance."

TM/HM slot and effect changes are public; locations are blind-friendly. Public example:

| TM/HM | Old move | New move |
| --- | --- | --- |
| TM12 | Taunt | Hurricane |

Blind-friendly entries may be explicit, but keep them in the spoiler-protected file. Make the first lines of that file clearly warn that it contains discovery spoilers.

When a single change has both aspects, write two entries. Example: a gym leader now uses the shared-power rule publicly, but the exact gym team belongs in blind-friendly notes.

## Implementation habits

- Use `rg` and `rg --files` to locate constants, generated files, and references before editing.
- Prefer source inputs over generated outputs. For example, edit `src/data/trainers.party`, not generated `src/data/trainers.h`.
- Follow existing data style and keep diffs focused.
- Add or update tests when mechanics, battle behavior, parser behavior, or generated contracts change.
- In final responses, mention which player-facing notes were updated and whether any blind-friendly details were intentionally kept out of the public summary.
