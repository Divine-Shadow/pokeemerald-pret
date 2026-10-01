# Reading waits and presentation gates

Reading controls attach to the actual narrative printer, independently of the battle script's wait command. A reveals a pane; R qualifies after ten reading frames, reveals four glyphs per renderer frame, and continues after a six-frame ready dwell. Neither input completes a controller or animation. Embedded timed pauses and WAIT_SE remain real waits in call and battle strings.

`waitreadmessage` identifies a reading delay explicitly. The initial 753 lexical candidates were narrowed to 695 standard long waits. Each retained wait has an immediately preceding printstring, printfromtable, critmessage, resultmessage, or attackstring in the same basic block, with no intervening entry label, directive, state mutation, or presentation operation. These producers issue a print controller and set MSG_DISPLAY when they produce a message. Crit/result producers may produce no message; the wait then follows the original no-message exit. Their controller flags remain the first synchronization boundary.

The new command waits for all battle controllers, the actual message printer, gAnimScriptActive, SE, and cries before accepting reading input. Its existing automatic reading timeout remains. Link and recorded battles, and the headless runner, use the original waitmessage counter algorithm rather than these new presentation/input gates. Explicit animation, controller, cry, fanfare, and popup commands were not converted.

The 58 additional exclusions retain short/medium pacing, cinematic and progression labels, capture outcomes, link-specific branches, and waits separated from a print by an entry label or directive. All numeric waits, every pause command, empty-string waits, and other waits without a direct producer also remain fixed. This conservative boundary still permits A/R to reveal their text, while preserving their final scripted delay.

Reviewed paths and retained boundaries:

- Capture: BallThrow emits handleballthrow without a new reading wait. ShakeBallThrow keeps animatewildpokemonafterfailedpokeball, waitstate, and the pre-result delay. Capture, trainer-block, dex, nickname, and caught-mon outcomes retain their original waits and menus.
- Evolution/form changes: mega, primal, ultra, Power Construct, Tera, and Dynamax reading delays remain fixed. All handleformchange, playanimation, waitanimation, and one-frame transition waits remain. Ordinary evolution and naming interfaces receive no changes.
- Switch/faint: move-switch short delays, return-to-ball controllers, party screens, switch animations, waitstate, and faint controllers remain. The multiple-faint loop keeps its switchineffects order and controller waits. Hazard/stat result messages can accept reading input only after their controllers and presentation complete.
- Moves: KnockedOff, SnatchedMove, and FocusPunchSetUp deliberately overlap text with playanimation; the new wait checks animation completion rather than relying on the old reading delay. All explicit move-animation and HP-bar controller waits remain. SE and cries can never be finished by A/R.
- Abilities: AbilityPopUp's show/pause/record ordering, fixed popup overwrite pauses, and destroy/waitabilitypopup commands remain. A global “all popups inactive” condition would deadlock fixed popups whose destruction follows the message. Intimidate retains its existing automatic timeout and uses the guarded input owner.
- Multi-battle: local double/spread result messages retain each controller and printer gate, and each new message starts a distinct input owner. Trainer slides, partner choices, and multi-faint party screens retain explicit synchronization. Link and recorded execution follow the legacy wait algorithm even for a shared script containing waitreadmessage.

Final confirmation cannot reuse A/B used by a printer in the same frame. Battle yes/no choices additionally wait for the prompt printer and reject that consumed edge. R continuation survives only a direct reading transition within four frames; resets at menu, scene cleanup, generic waits, or deliberate pauses discard it.

Validation evidence and interactive limits are recorded in [.agent/dialogue_reading_execplan.md](../../.agent/dialogue_reading_execplan.md).
