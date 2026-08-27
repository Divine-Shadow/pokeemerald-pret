Common_EventScript_ConditionCoach::
	lock
	faceplayer
	msgbox ConditionCoach_Text_Intro, MSGBOX_DEFAULT
	getpartysize
	goto_if_eq VAR_RESULT, 0, ConditionCoach_EventScript_NoPokemon

ConditionCoach_EventScript_Menu:
	msgbox ConditionCoach_Text_MenuPrompt, MSGBOX_DEFAULT
	dynmultipush ConditionCoach_Text_MenuPoison, CONDITION_COACH_CHOICE_POISON
	dynmultipush ConditionCoach_Text_MenuBurn, CONDITION_COACH_CHOICE_BURN
	dynmultipush ConditionCoach_Text_MenuParalysis, CONDITION_COACH_CHOICE_PARALYSIS
	dynmultipush ConditionCoach_Text_MenuRestWake, CONDITION_COACH_CHOICE_REST_WAKE
	dynmultipush ConditionCoach_Text_MenuClear, CONDITION_COACH_CHOICE_CLEAR
	dynmultipush ConditionCoach_Text_MenuExit, CONDITION_COACH_CHOICE_EXIT
	dynmultistack 0, 0, FALSE, 6, FALSE, CONDITION_COACH_CHOICE_POISON, NULL
	switch VAR_RESULT
	case CONDITION_COACH_CHOICE_POISON, ConditionCoach_EventScript_SelectPoison
	case CONDITION_COACH_CHOICE_BURN, ConditionCoach_EventScript_SelectBurn
	case CONDITION_COACH_CHOICE_PARALYSIS, ConditionCoach_EventScript_SelectParalysis
	case CONDITION_COACH_CHOICE_REST_WAKE, ConditionCoach_EventScript_SelectRestWake
	case CONDITION_COACH_CHOICE_CLEAR, ConditionCoach_EventScript_SelectClear
	case CONDITION_COACH_CHOICE_EXIT, ConditionCoach_EventScript_Goodbye
	case MULTI_B_PRESSED, ConditionCoach_EventScript_Goodbye
	goto ConditionCoach_EventScript_Goodbye

ConditionCoach_EventScript_SelectBurn:
	setvar VAR_0x8005, CONDITION_COACH_CHOICE_BURN
	specialvar VAR_RESULT, ConditionCoach_IsChoiceUnlocked
	goto_if_eq VAR_RESULT, FALSE, ConditionCoach_EventScript_Locked
	goto ConditionCoach_EventScript_SelectMon

ConditionCoach_EventScript_SelectPoison:
	setvar VAR_0x8005, CONDITION_COACH_CHOICE_POISON
	specialvar VAR_RESULT, ConditionCoach_IsChoiceUnlocked
	goto_if_eq VAR_RESULT, FALSE, ConditionCoach_EventScript_Locked
	goto ConditionCoach_EventScript_SelectMon

ConditionCoach_EventScript_SelectParalysis:
	setvar VAR_0x8005, CONDITION_COACH_CHOICE_PARALYSIS
	specialvar VAR_RESULT, ConditionCoach_IsChoiceUnlocked
	goto_if_eq VAR_RESULT, FALSE, ConditionCoach_EventScript_Locked
	goto ConditionCoach_EventScript_SelectMon

ConditionCoach_EventScript_SelectRestWake:
	setvar VAR_0x8005, CONDITION_COACH_CHOICE_REST_WAKE
	specialvar VAR_RESULT, ConditionCoach_IsChoiceUnlocked
	goto_if_eq VAR_RESULT, FALSE, ConditionCoach_EventScript_Locked
	msgbox ConditionCoach_Text_RestWakeExplain, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_SelectMon

ConditionCoach_EventScript_SelectClear:
	setvar VAR_0x8005, CONDITION_COACH_CHOICE_CLEAR
	specialvar VAR_RESULT, ConditionCoach_IsChoiceUnlocked
	goto_if_eq VAR_RESULT, FALSE, ConditionCoach_EventScript_Locked
	goto ConditionCoach_EventScript_SelectMon

ConditionCoach_EventScript_SelectMon:
	special ChooseConditionCoachPartyMon
	waitstate
	goto_if_eq VAR_0x8004, CONDITION_COACH_PARTY_ALL, ConditionCoach_EventScript_PreviewParty
	goto_if_ge VAR_0x8004, PARTY_SIZE, ConditionCoach_EventScript_Cancelled
	bufferpartymonnick STR_VAR_1, VAR_0x8004
	specialvar VAR_RESULT, ConditionCoach_TryApplyStatus
	switch VAR_RESULT
	case CONDITION_COACH_RESULT_APPLIED, ConditionCoach_EventScript_Applied
	case CONDITION_COACH_RESULT_CANCELLED, ConditionCoach_EventScript_Cancelled
	case CONDITION_COACH_RESULT_EGG, ConditionCoach_EventScript_Egg
	case CONDITION_COACH_RESULT_FAINTED, ConditionCoach_EventScript_Fainted
	case CONDITION_COACH_RESULT_ALREADY_STATUS, ConditionCoach_EventScript_AlreadyStatus
	case CONDITION_COACH_RESULT_ALREADY_CLEAR, ConditionCoach_EventScript_AlreadyClear
	case CONDITION_COACH_RESULT_INVALID_CHOICE, ConditionCoach_EventScript_Goodbye
	case CONDITION_COACH_RESULT_LOCKED, ConditionCoach_EventScript_Locked
	goto ConditionCoach_EventScript_Goodbye

ConditionCoach_EventScript_PreviewParty:
	specialvar VAR_RESULT, ConditionCoach_TryPreviewParty
	switch VAR_RESULT
	case CONDITION_COACH_RESULT_PARTY_READY, ConditionCoach_EventScript_ConfirmParty
	case CONDITION_COACH_RESULT_ALREADY_CLEAR, ConditionCoach_EventScript_PartyAlreadyClear
	case CONDITION_COACH_RESULT_NO_ELIGIBLE, ConditionCoach_EventScript_NoEligibleParty
	case CONDITION_COACH_RESULT_INVALID_CHOICE, ConditionCoach_EventScript_Goodbye
	case CONDITION_COACH_RESULT_LOCKED, ConditionCoach_EventScript_Locked
	goto ConditionCoach_EventScript_Goodbye

ConditionCoach_EventScript_ConfirmParty:
	msgbox gStringVar4, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, ConditionCoach_EventScript_Menu
	specialvar VAR_RESULT, ConditionCoach_TryApplyStatusToParty
	switch VAR_RESULT
	case CONDITION_COACH_RESULT_APPLIED, ConditionCoach_EventScript_PartyApplied
	case CONDITION_COACH_RESULT_ALREADY_CLEAR, ConditionCoach_EventScript_PartyAlreadyClear
	case CONDITION_COACH_RESULT_NO_ELIGIBLE, ConditionCoach_EventScript_NoEligibleParty
	case CONDITION_COACH_RESULT_INVALID_CHOICE, ConditionCoach_EventScript_Goodbye
	case CONDITION_COACH_RESULT_LOCKED, ConditionCoach_EventScript_Locked
	goto ConditionCoach_EventScript_Goodbye

ConditionCoach_EventScript_PartyApplied:
	msgbox gStringVar4, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_PartyAlreadyClear:
	msgbox ConditionCoach_Text_PartyAlreadyClear, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_NoEligibleParty:
	msgbox ConditionCoach_Text_NoEligibleParty, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_Locked:
	buffernumberstring STR_VAR_1, VAR_0x8006
	msgbox ConditionCoach_Text_Locked, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_Applied:
	msgbox ConditionCoach_Text_Applied, MSGBOX_DEFAULT
	switch VAR_0x8006
	case CONDITION_COACH_HINT_CURING_ITEM, ConditionCoach_EventScript_HintCuringItem
	case CONDITION_COACH_HINT_GUTS, ConditionCoach_EventScript_HintGuts
	case CONDITION_COACH_HINT_MARVEL_SCALE, ConditionCoach_EventScript_HintMarvelScale
	case CONDITION_COACH_HINT_QUICK_FEET, ConditionCoach_EventScript_HintQuickFeet
	case CONDITION_COACH_HINT_POISON_HEAL, ConditionCoach_EventScript_HintPoisonHeal
	case CONDITION_COACH_HINT_TOXIC_BOOST, ConditionCoach_EventScript_HintToxicBoost
	case CONDITION_COACH_HINT_FLARE_BOOST, ConditionCoach_EventScript_HintFlareBoost
	case CONDITION_COACH_HINT_MAGIC_GUARD, ConditionCoach_EventScript_HintMagicGuard
	case CONDITION_COACH_HINT_STATUS_MAY_SLIP, ConditionCoach_EventScript_HintStatusMaySlip
	case CONDITION_COACH_HINT_REST_WAKE, ConditionCoach_EventScript_HintRestWake
	case CONDITION_COACH_HINT_CLEAR, ConditionCoach_EventScript_HintClear
	msgbox ConditionCoach_Text_HintDefault, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintCuringItem:
	msgbox ConditionCoach_Text_HintCuringItem, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintGuts:
	msgbox ConditionCoach_Text_HintGuts, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintMarvelScale:
	msgbox ConditionCoach_Text_HintMarvelScale, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintQuickFeet:
	msgbox ConditionCoach_Text_HintQuickFeet, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintPoisonHeal:
	msgbox ConditionCoach_Text_HintPoisonHeal, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintToxicBoost:
	msgbox ConditionCoach_Text_HintToxicBoost, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintFlareBoost:
	msgbox ConditionCoach_Text_HintFlareBoost, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintMagicGuard:
	msgbox ConditionCoach_Text_HintMagicGuard, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintStatusMaySlip:
	msgbox ConditionCoach_Text_HintStatusMaySlip, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintRestWake:
	msgbox ConditionCoach_Text_HintRestWake, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_HintClear:
	msgbox ConditionCoach_Text_HintClear, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_Cancelled:
	msgbox ConditionCoach_Text_Cancelled, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_Egg:
	msgbox ConditionCoach_Text_Egg, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_Fainted:
	msgbox ConditionCoach_Text_Fainted, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_AlreadyStatus:
	msgbox ConditionCoach_Text_AlreadyStatus, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_AlreadyClear:
	msgbox ConditionCoach_Text_AlreadyClear, MSGBOX_DEFAULT
	goto ConditionCoach_EventScript_Menu

ConditionCoach_EventScript_NoPokemon:
	msgbox ConditionCoach_Text_NoPokemon, MSGBOX_DEFAULT
	release
	end

ConditionCoach_EventScript_Goodbye:
	msgbox ConditionCoach_Text_Goodbye, MSGBOX_DEFAULT
	release
	end

ConditionCoach_Text_Intro:
	.string "Hi. I'm the CONDITION COACH.\p"
	.string "The Nurse restores conditions.\n"
	.string "I help Trainers prepare them\l"
	.string "safely for battle.$"

ConditionCoach_Text_MenuPrompt:
	.string "What condition should we\n"
	.string "prepare today?$"

ConditionCoach_Text_MenuBurn:
	.string "Burn$"

ConditionCoach_Text_MenuPoison:
	.string "Poison$"

ConditionCoach_Text_MenuParalysis:
	.string "Paralysis$"

ConditionCoach_Text_MenuRestWake:
	.string "Rest-wake$"

ConditionCoach_Text_MenuClear:
	.string "Clear status$"

ConditionCoach_Text_MenuExit:
	.string "Exit$"

ConditionCoach_Text_RestWakeExplain:
	.string "Rest-wake sets a light sleep.\p"
	.string "It should wake up as soon as\n"
	.string "it tries to act in battle.$"

ConditionCoach_Text_Applied:
	.string "Done. {STR_VAR_1}'s condition\n"
	.string "is set.$"

ConditionCoach_Text_PartyAlreadyClear:
	.string "Every eligible POKéMON is\n"
	.string "already clear.$"

ConditionCoach_Text_NoEligibleParty:
	.string "I couldn't find a conscious,\n"
	.string "non-Egg teammate to prepare.$"

ConditionCoach_Text_HintCuringItem:
	.string "One caution: that held berry\n"
	.string "will cure the condition fast.\p"
	.string "That may still be your plan.$"

ConditionCoach_Text_HintGuts:
	.string "That one's built for it.\n"
	.string "GUTS turns strain into damage.$"

ConditionCoach_Text_HintMarvelScale:
	.string "MARVEL SCALE responds well\n"
	.string "to controlled pressure.\p"
	.string "Use the extra bulk wisely.$"

ConditionCoach_Text_HintQuickFeet:
	.string "QUICK FEET turns pressure\n"
	.string "into pace.\p"
	.string "Pick the condition your plan\n"
	.string "can carry.$"

ConditionCoach_Text_HintPoisonHeal:
	.string "POISON HEAL turns poison\n"
	.string "into steady recovery.\p"
	.string "Protect that condition.$"

ConditionCoach_Text_HintToxicBoost:
	.string "TOXIC BOOST wants poison\n"
	.string "on schedule.\p"
	.string "Now the clock is yours.$"

ConditionCoach_Text_HintFlareBoost:
	.string "FLARE BOOST wants burn\n"
	.string "already handled.\p"
	.string "Now it can focus.$"

ConditionCoach_Text_HintMagicGuard:
	.string "MAGIC GUARD keeps the usual\n"
	.string "chip damage quiet.\p"
	.string "The condition still matters.$"

ConditionCoach_Text_HintStatusMaySlip:
	.string "One caution: this plan has\n"
	.string "its own cleanup habits.\p"
	.string "If it slips, that may be\n"
	.string "the clue you needed.$"

ConditionCoach_Text_HintRestWake:
	.string "This is the last beat of REST,\n"
	.string "not a full nap.\p"
	.string "It should wake on action.$"

ConditionCoach_Text_HintClear:
	.string "Clean slate. Now the Nurse\n"
	.string "and I agree.$"

ConditionCoach_Text_HintDefault:
	.string "Keep the tradeoff in mind.\p"
	.string "A condition only helps when\n"
	.string "your team converts it.$"

ConditionCoach_Text_Cancelled:
	.string "No problem. Good prep starts\n"
	.string "with choosing the right target.$"

ConditionCoach_Text_Egg:
	.string "No drills for Eggs.\p"
	.string "They get rest, warmth,\n"
	.string "and nothing else.$"

ConditionCoach_Text_Fainted:
	.string "{STR_VAR_1} needs normal care\n"
	.string "before condition training.\p"
	.string "Please see the Nurse first.$"

ConditionCoach_Text_AlreadyStatus:
	.string "{STR_VAR_1} already has a\n"
	.string "condition.\p"
	.string "Clear it first if you want\n"
	.string "a different setup.$"

ConditionCoach_Text_AlreadyClear:
	.string "{STR_VAR_1} is already clear.\n"
	.string "No adjustment needed.$"

ConditionCoach_Text_Locked:
	.string "That drill opens once you\n"
	.string "have {STR_VAR_1} GYM BADGES.$"

ConditionCoach_Text_NoPokemon:
	.string "Bring me a conscious POKéMON\n"
	.string "and we can run a controlled\l"
	.string "condition drill.$"

ConditionCoach_Text_Goodbye:
	.string "Condition is a tool.\n"
	.string "Use it with a plan.$"
