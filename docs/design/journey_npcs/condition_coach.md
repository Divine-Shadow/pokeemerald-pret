# Condition Coach

## Concept

The Condition Coach is a recurring Pokemon Center specialist who treats major status conditions as a controlled battle-prep tool. The Nurse restores Pokemon to baseline; the Coach helps a trainer intentionally prepare burn, poison, paralysis, or a Rest-style wakeup state when the team has a plan for it.

## Player Impact

The coach removes wild-encounter busywork for teams built around Guts, Quick Feet, Marvel Scale, Poison Heal, Toxic Boost, Flare Boost, Facade, and similar status-aware plans. Players can use the service before important fights instead of fishing for random status moves.

The service is also a teaching surface. Early players can learn that status is normally a drawback. More advanced players get confirmation that specific abilities and held berries change that tradeoff.

## Service Rules

- Burn, regular poison, paralysis, Rest-wake sleep, and clear status are available from the first MVP pass.
- Rest-wake sleep uses a one-turn sleep counter, matching the end of a Rest cycle where the Pokemon should wake as soon as it tries to act.
- The coach does not status Eggs or fainted Pokemon.
- Applying a condition replaces an existing major status.
- On the party screen, A prepares the highlighted Pokemon, START offers to prepare every eligible party member after confirmation, and B returns without applying anything.
- Whole-party preparation skips Eggs and fainted Pokemon, reports how many party members were prepared, and condenses distinct ability synergies and cautions into one optional feedback pane.
- The coach warns when a held curing berry would immediately undo the selected condition.

## Journey Beats

- Early beat: the coach explains that Pokemon Centers heal conditions while the coach teaches safe preparation.
- Middle beat: the coach recognizes that some teams convert status into speed, bulk, damage, or recovery.
- Late beat: the coach supports whole-party preparation and can become still more technical with bad poison or more precise Rest/Chesto timing if those become desirable.
- Postgame beat: the coach can support competitive-condition presets and deeper advice for status-triggered items or abilities.

## Tailored Text Categories

The MVP recognizes these cases after a successful selection:

- Held status-curing berries that will cure the selected status.
- Guts with burn, poison, or paralysis.
- Marvel Scale with any applied major status.
- Quick Feet with burn, poison, paralysis, or Rest-wake sleep.
- Poison Heal with poison.
- Toxic Boost with poison.
- Flare Boost with burn.
- Magic Guard with any damaging status.
- Natural Cure, Shed Skin, and Hydration with any applied status, using a subtle caution rather than a direct mechanics explanation.
- Rest-wake sleep.
- Clear status.

Ignored status-adjacent abilities:

- Heatproof, because its burn damage reduction is intentionally obscure.
- Synchronize, because pre-status from the coach is not the moment that ability teaches.

## Asset Direction

The overworld sprite should read as a clinical sports trainer rather than as another Nurse. The MVP asset uses a distinct teal, white, and red palette over a practical expert silhouette so it belongs in a Pokemon Center while still being easy to distinguish at a glance.

Current planned asset id:

- Object event gfx: `OBJ_EVENT_GFX_CONDITION_COACH`

## Sprite Proof

The coach uses the journey NPC sprite preview command and the same 16x32 object-event sheet conversion rule as Barry.

Latest local proof artifacts:

- `build/journey_npc_sprites/condition_coach/run_result.json`
- `build/journey_npc_sprites/condition_coach/condition_coach_all.png`
- `build/journey_npc_sprites/condition_coach/condition_coach_overworld_south.png`
- `build/journey_npc_sprites/condition_coach/condition_coach_overworld_north.png`
- `build/journey_npc_sprites/condition_coach/condition_coach_overworld_west.png`
- `build/journey_npc_sprites/condition_coach/condition_coach_overworld_east.png`

## Placement Status

The MVP places one coach in every 1F Pokemon Center. Exact dialogue and placement are blind-friendly details, while the broad existence of the service is public because it changes team planning.
