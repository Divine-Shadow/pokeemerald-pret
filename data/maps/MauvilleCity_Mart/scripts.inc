MauvilleCity_Mart_MapScripts::
	.byte 0

MauvilleCity_Mart_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart MauvilleCity_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

MauvilleCity_Mart_EventScript_WildHeldItemBroker::
	lock
	faceplayer
	setvar VAR_TEMP_0, WILD_HELD_ITEM_BROKER_CATEGORY_HELD_GEAR
	goto_if_set FLAG_WILD_HELD_ITEM_BROKER_UNLOCKED, MauvilleCity_Mart_EventScript_WildHeldItemBrokerCategoryMenu
	msgbox MauvilleCity_Mart_Text_BrokerIntro, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, MauvilleCity_Mart_EventScript_WildHeldItemBrokerDecline
	msgbox MauvilleCity_Mart_Text_BrokerChooseMon, MSGBOX_DEFAULT
	special ChoosePartyMon
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, MauvilleCity_Mart_EventScript_WildHeldItemBrokerDecline
	specialvar VAR_RESULT, Script_IsWildHeldItemBrokerEligiblePartyMon
	goto_if_eq VAR_RESULT, TRUE, MauvilleCity_Mart_EventScript_WildHeldItemBrokerUnlock
	msgbox MauvilleCity_Mart_Text_BrokerNotEligible, MSGBOX_DEFAULT
	release
	end

MauvilleCity_Mart_EventScript_WildHeldItemBrokerUnlock::
	setflag FLAG_WILD_HELD_ITEM_BROKER_UNLOCKED
	msgbox MauvilleCity_Mart_Text_BrokerUnlock, MSGBOX_DEFAULT
	goto MauvilleCity_Mart_EventScript_WildHeldItemBrokerCategoryMenu

MauvilleCity_Mart_EventScript_WildHeldItemBrokerCategoryMenu::
	closemessage
	dynmultipush MauvilleCity_Mart_Text_BrokerMenuHeldGear, WILD_HELD_ITEM_BROKER_CATEGORY_HELD_GEAR
	dynmultipush MauvilleCity_Mart_Text_BrokerMenuOneUseHolds, WILD_HELD_ITEM_BROKER_CATEGORY_ONE_USE_HOLDS
	dynmultipush MauvilleCity_Mart_Text_BrokerMenuTypeBoosts, WILD_HELD_ITEM_BROKER_CATEGORY_TYPE_BOOSTS
	dynmultipush MauvilleCity_Mart_Text_BrokerMenuEvolutionForm, WILD_HELD_ITEM_BROKER_CATEGORY_EVOLUTION_FORM
	dynmultipush MauvilleCity_Mart_Text_BrokerMenuMedicine, WILD_HELD_ITEM_BROKER_CATEGORY_MEDICINE
	dynmultipush MauvilleCity_Mart_Text_BrokerMenuResources, WILD_HELD_ITEM_BROKER_CATEGORY_RESOURCES
	dynmultipush MauvilleCity_Mart_Text_BrokerMenuAllStock, WILD_HELD_ITEM_BROKER_CATEGORY_ALL
	dynmultipush MauvilleCity_Mart_Text_BrokerMenuLeave, WILD_HELD_ITEM_BROKER_CATEGORY_EXIT
	dynmultistack 0, 0, FALSE, 8, FALSE, VAR_TEMP_0, DYN_MULTICHOICE_CB_NONE
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, MauvilleCity_Mart_EventScript_WildHeldItemBrokerExit
	goto_if_ge VAR_RESULT, WILD_HELD_ITEM_BROKER_CATEGORY_EXIT, MauvilleCity_Mart_EventScript_WildHeldItemBrokerExit
	copyvar VAR_TEMP_0, VAR_RESULT
	copyvar VAR_0x8004, VAR_RESULT
	special OpenWildHeldItemBrokerShop
	goto MauvilleCity_Mart_EventScript_WildHeldItemBrokerCategoryMenu

MauvilleCity_Mart_EventScript_WildHeldItemBrokerExit::
	closemessage
	release
	end

MauvilleCity_Mart_EventScript_WildHeldItemBrokerDecline::
	msgbox MauvilleCity_Mart_Text_BrokerDecline, MSGBOX_DEFAULT
	release
	end

	.align 2
MauvilleCity_Mart_Pokemart:
	.2byte ITEM_POKE_BALL
	.2byte ITEM_GREAT_BALL
	.2byte ITEM_SUPER_POTION
	.2byte ITEM_ANTIDOTE
	.2byte ITEM_PARALYZE_HEAL
	.2byte ITEM_AWAKENING
	.2byte ITEM_X_SPEED
	.2byte ITEM_X_ATTACK
	.2byte ITEM_X_DEFENSE
	.2byte ITEM_GUARD_SPEC
	.2byte ITEM_DIRE_HIT
	.2byte ITEM_X_ACCURACY
	pokemartlistend

MauvilleCity_Mart_EventScript_ExpertM::
	msgbox MauvilleCity_Mart_Text_ItemsToTemporarilyElevateStats, MSGBOX_NPC
	end

MauvilleCity_Mart_EventScript_Man::
	msgbox MauvilleCity_Mart_Text_DecisionsDetermineBattle, MSGBOX_NPC
	end

MauvilleCity_Mart_Text_ItemsToTemporarilyElevateStats:
	.string "There are items that temporarily\n"
	.string "elevate the stats of POKéMON.\p"
	.string "The ones I know you use in battle\n"
	.string "are X ATTACK and X DEFEND…\p"
	.string "I do believe that there are others\n"
	.string "like them.$"

MauvilleCity_Mart_Text_DecisionsDetermineBattle:
	.string "Use a certain move, or use an item\n"
	.string "instead…\p"
	.string "The TRAINER's decisions determine how\n"
	.string "battles turn out, I think.$"

MauvilleCity_Mart_Text_BrokerIntro:
	.string "Wild POKéMON carry little fortunes.\n"
	.string "I made mine lifting them clean.\p"
	.string "Last week, a one-percent prize paid\n"
	.string "for this suit.\p"
	.string "Do you have a partner with that kind\n"
	.string "of touch?$"

MauvilleCity_Mart_Text_BrokerChooseMon:
	.string "Show me a POKéMON with a move or\n"
	.string "ABILITY that can take or swap held\l"
	.string "items.$"

MauvilleCity_Mart_Text_BrokerNotEligible:
	.string "Hm. That one doesn't have the touch.\p"
	.string "I mean THIEF, COVET, TRICK,\n"
	.string "SWITCHEROO, or an ABILITY that\l"
	.string "pockets goods.$"

MauvilleCity_Mart_Text_BrokerUnlock:
	.string "That's the look.\p"
	.string "No need to bother wild POKéMON all\n"
	.string "day when we both know how it ends.\p"
	.string "My stock is yours. No charge.$"

MauvilleCity_Mart_Text_BrokerMenuHeldGear:
	.string "HELD GEAR$"

MauvilleCity_Mart_Text_BrokerMenuOneUseHolds:
	.string "ONE-USE HOLDS$"

MauvilleCity_Mart_Text_BrokerMenuTypeBoosts:
	.string "TYPE BOOSTS$"

MauvilleCity_Mart_Text_BrokerMenuEvolutionForm:
	.string "EVOLUTION/FORM$"

MauvilleCity_Mart_Text_BrokerMenuMedicine:
	.string "MEDICINE$"

MauvilleCity_Mart_Text_BrokerMenuResources:
	.string "RESOURCES$"

MauvilleCity_Mart_Text_BrokerMenuAllStock:
	.string "ALL STOCK$"

MauvilleCity_Mart_Text_BrokerMenuLeave:
	.string "LEAVE$"

MauvilleCity_Mart_Text_BrokerDecline:
	.string "No rush.\p"
	.string "Come back when your party knows\n"
	.string "the trade.$"
