#include "global.h"
#include "event_data.h"
#include "pokemon.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "test/test.h"
#include "constants/abilities.h"
#include "constants/battle.h"
#include "constants/condition_coach.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/species.h"

static void SetConditionCoachBadgeCount(u8 badgeCount)
{
    u8 i;

    for (i = 0; i < NUM_BADGES; i++)
        FlagClear(gBadgeFlags[i]);

    for (i = 0; i < badgeCount; i++)
        FlagSet(gBadgeFlags[i]);
}

static void AddConditionCoachTestMon(u8 slot, u16 species, u32 abilityNum)
{
    CreateMon(&gPlayerParty[slot], species, 50, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
    SetMonData(&gPlayerParty[slot], MON_DATA_ABILITY_NUM, &abilityNum);
}

static void CreateConditionCoachTestMon(u16 species)
{
    ZeroPlayerPartyMons();
    AddConditionCoachTestMon(0, species, 0);
    CalculatePlayerPartyCount();
    gSpecialVar_0x8004 = 0;
    gSpecialVar_0x8005 = CONDITION_COACH_CHOICE_BURN;
    gSpecialVar_0x8006 = CONDITION_COACH_HINT_NONE;
    SetConditionCoachBadgeCount(NUM_BADGES);
}

static u16 TryConditionCoachChoice(u16 choice)
{
    gSpecialVar_0x8005 = choice;
    return ConditionCoach_TryApplyStatus();
}

static u16 TryConditionCoachPartyChoice(u16 choice)
{
    gSpecialVar_0x8005 = choice;
    return ConditionCoach_TryApplyStatusToParty();
}

TEST("Condition Coach applies burn")
{
    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_BURN), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_BURN);
}

TEST("Condition Coach requires two badges for burn")
{
    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);
    SetConditionCoachBadgeCount(1);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_BURN), CONDITION_COACH_RESULT_LOCKED);
    EXPECT_EQ(gSpecialVar_0x8006, 2);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_NONE);

    SetConditionCoachBadgeCount(2);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_BURN), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_BURN);
}

TEST("Condition Coach applies regular poison")
{
    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);
    SetConditionCoachBadgeCount(0);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_POISON), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_POISON);
}

TEST("Condition Coach applies paralysis")
{
    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_PARALYSIS), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_PARALYSIS);
}

TEST("Condition Coach requires four badges for paralysis")
{
    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);
    SetConditionCoachBadgeCount(3);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_PARALYSIS), CONDITION_COACH_RESULT_LOCKED);
    EXPECT_EQ(gSpecialVar_0x8006, 4);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_NONE);

    SetConditionCoachBadgeCount(4);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_PARALYSIS), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_PARALYSIS);
}

TEST("Condition Coach applies one-turn Rest-wake sleep")
{
    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_REST_WAKE), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_SLEEP_TURN(1));
    EXPECT_EQ(gSpecialVar_0x8006, CONDITION_COACH_HINT_REST_WAKE);
}

TEST("Condition Coach requires six badges for Rest-wake sleep")
{
    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);
    SetConditionCoachBadgeCount(5);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_REST_WAKE), CONDITION_COACH_RESULT_LOCKED);
    EXPECT_EQ(gSpecialVar_0x8006, 6);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_NONE);

    SetConditionCoachBadgeCount(6);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_REST_WAKE), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_SLEEP_TURN(1));
}

TEST("Condition Coach clears status")
{
    u32 status = STATUS1_BURN;

    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);
    SetConditionCoachBadgeCount(0);
    SetMonData(&gPlayerParty[0], MON_DATA_STATUS, &status);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_CLEAR), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_NONE);
    EXPECT_EQ(gSpecialVar_0x8006, CONDITION_COACH_HINT_CLEAR);
}

TEST("Condition Coach reports already-clear Pokemon")
{
    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_CLEAR), CONDITION_COACH_RESULT_ALREADY_CLEAR);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_NONE);
}

TEST("Condition Coach overwrites existing status")
{
    u32 status = STATUS1_BURN;

    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);
    SetMonData(&gPlayerParty[0], MON_DATA_STATUS, &status);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_POISON), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_POISON);
}

TEST("Condition Coach rejects Eggs")
{
    u32 isEgg = TRUE;

    ZeroPlayerPartyMons();
    CreateMon(&gPlayerParty[0], SPECIES_PICHU, 5, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
    SetMonData(&gPlayerParty[0], MON_DATA_IS_EGG, &isEgg);
    CalculatePlayerPartyCount();
    gSpecialVar_0x8004 = 0;
    gSpecialVar_0x8005 = CONDITION_COACH_CHOICE_BURN;
    gSpecialVar_0x8006 = CONDITION_COACH_HINT_NONE;
    SetConditionCoachBadgeCount(NUM_BADGES);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_BURN), CONDITION_COACH_RESULT_EGG);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_NONE);
}

TEST("Condition Coach rejects fainted Pokemon")
{
    u32 hp = 0;

    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);
    SetMonData(&gPlayerParty[0], MON_DATA_HP, &hp);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_BURN), CONDITION_COACH_RESULT_FAINTED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_NONE);
}

TEST("Condition Coach warns when a held berry will cure the selected condition")
{
    u32 item = ITEM_RAWST_BERRY;

    CreateConditionCoachTestMon(SPECIES_WOBBUFFET);
    SetMonData(&gPlayerParty[0], MON_DATA_HELD_ITEM, &item);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_BURN), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(gSpecialVar_0x8006, CONDITION_COACH_HINT_CURING_ITEM);
}

TEST("Condition Coach gives Guts tailored advice")
{
    CreateConditionCoachTestMon(SPECIES_MACHOP);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_BURN), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(gSpecialVar_0x8006, CONDITION_COACH_HINT_GUTS);
}

TEST("Condition Coach gives Quick Feet tailored advice")
{
    u32 abilityNum = 1;

    CreateConditionCoachTestMon(SPECIES_POOCHYENA);
    SetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM, &abilityNum);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_PARALYSIS), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(gSpecialVar_0x8006, CONDITION_COACH_HINT_QUICK_FEET);
}

TEST("Condition Coach gives Quick Feet advice for every status prep")
{
    u32 abilityNum = 1;

    CreateConditionCoachTestMon(SPECIES_POOCHYENA);
    SetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM, &abilityNum);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_BURN), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(gSpecialVar_0x8006, CONDITION_COACH_HINT_QUICK_FEET);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_CLEAR), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_POISON), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(gSpecialVar_0x8006, CONDITION_COACH_HINT_QUICK_FEET);

    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_CLEAR), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(TryConditionCoachChoice(CONDITION_COACH_CHOICE_REST_WAKE), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(gSpecialVar_0x8006, CONDITION_COACH_HINT_QUICK_FEET);
}

TEST("Condition Coach foreshadows abilities that may clean up status")
{
    u16 species, ability, choice;

    PARAMETRIZE { species = SPECIES_CHANSEY; ability = ABILITY_NATURAL_CURE; choice = CONDITION_COACH_CHOICE_POISON; }
    PARAMETRIZE { species = SPECIES_SILCOON; ability = ABILITY_SHED_SKIN; choice = CONDITION_COACH_CHOICE_BURN; }
    PARAMETRIZE { species = SPECIES_PHIONE; ability = ABILITY_HYDRATION; choice = CONDITION_COACH_CHOICE_REST_WAKE; }

    ASSUME(GetSpeciesAbility(species, 0) == ability);

    CreateConditionCoachTestMon(species);

    EXPECT_EQ(TryConditionCoachChoice(choice), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(gSpecialVar_0x8006, CONDITION_COACH_HINT_STATUS_MAY_SLIP);
}

TEST("Condition Coach ignores Heatproof and Synchronize for status advice")
{
    u16 species, ability, choice;
    u32 abilityNum = 0;

    PARAMETRIZE { species = SPECIES_BRONZOR; ability = ABILITY_HEATPROOF; abilityNum = 1; choice = CONDITION_COACH_CHOICE_BURN; }
    PARAMETRIZE { species = SPECIES_ABRA; ability = ABILITY_SYNCHRONIZE; abilityNum = 0; choice = CONDITION_COACH_CHOICE_POISON; }

    ASSUME(GetSpeciesAbility(species, abilityNum) == ability);

    CreateConditionCoachTestMon(species);
    SetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM, &abilityNum);

    EXPECT_EQ(TryConditionCoachChoice(choice), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(gSpecialVar_0x8006, CONDITION_COACH_HINT_NONE);
}

TEST("Condition Coach previews the whole eligible party without changing status")
{
    u32 status = STATUS1_BURN;

    ZeroPlayerPartyMons();
    AddConditionCoachTestMon(0, SPECIES_WOBBUFFET, 0);
    AddConditionCoachTestMon(1, SPECIES_MACHOP, 0);
    SetMonData(&gPlayerParty[1], MON_DATA_STATUS, &status);
    CalculatePlayerPartyCount();
    SetConditionCoachBadgeCount(NUM_BADGES);
    gSpecialVar_0x8005 = CONDITION_COACH_CHOICE_POISON;

    EXPECT_EQ(ConditionCoach_TryPreviewParty(), CONDITION_COACH_RESULT_PARTY_READY);
    EXPECT_EQ(gSpecialVar_0x8007, 2);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_NONE);
    EXPECT_EQ(GetMonData(&gPlayerParty[1], MON_DATA_STATUS), STATUS1_BURN);
}

TEST("Condition Coach applies a condition to every eligible party member")
{
    u32 status = STATUS1_BURN;
    u32 isEgg = TRUE;
    u32 hp = 0;

    ZeroPlayerPartyMons();
    AddConditionCoachTestMon(0, SPECIES_WOBBUFFET, 0);
    AddConditionCoachTestMon(1, SPECIES_MACHOP, 0);
    AddConditionCoachTestMon(2, SPECIES_PICHU, 0);
    AddConditionCoachTestMon(3, SPECIES_WOBBUFFET, 0);
    SetMonData(&gPlayerParty[1], MON_DATA_STATUS, &status);
    SetMonData(&gPlayerParty[2], MON_DATA_IS_EGG, &isEgg);
    SetMonData(&gPlayerParty[3], MON_DATA_HP, &hp);
    CalculatePlayerPartyCount();
    SetConditionCoachBadgeCount(NUM_BADGES);

    EXPECT_EQ(TryConditionCoachPartyChoice(CONDITION_COACH_CHOICE_POISON), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_POISON);
    EXPECT_EQ(GetMonData(&gPlayerParty[1], MON_DATA_STATUS), STATUS1_POISON);
    EXPECT_EQ(GetMonData(&gPlayerParty[2], MON_DATA_STATUS), STATUS1_NONE);
    EXPECT_EQ(GetMonData(&gPlayerParty[3], MON_DATA_STATUS), STATUS1_NONE);
    EXPECT_EQ(gSpecialVar_0x8007, 2);
    EXPECT_EQ(gSpecialVar_0x8008, 1);
    EXPECT_EQ(gSpecialVar_0x8009, 1);
    EXPECT(gSpecialVar_0x800B & CONDITION_COACH_SIGNAL_GUTS);
}

TEST("Condition Coach clears status from the whole party and counts no-op targets")
{
    u32 burn = STATUS1_BURN;
    u32 poison = STATUS1_POISON;

    ZeroPlayerPartyMons();
    AddConditionCoachTestMon(0, SPECIES_WOBBUFFET, 0);
    AddConditionCoachTestMon(1, SPECIES_MACHOP, 0);
    AddConditionCoachTestMon(2, SPECIES_PICHU, 0);
    SetMonData(&gPlayerParty[0], MON_DATA_STATUS, &burn);
    SetMonData(&gPlayerParty[1], MON_DATA_STATUS, &poison);
    CalculatePlayerPartyCount();
    SetConditionCoachBadgeCount(0);
    gSpecialVar_0x8005 = CONDITION_COACH_CHOICE_CLEAR;

    EXPECT_EQ(ConditionCoach_TryPreviewParty(), CONDITION_COACH_RESULT_PARTY_READY);
    EXPECT_EQ(gSpecialVar_0x8007, 2);
    EXPECT(StringCompare(gStringVar4, COMPOUND_STRING("Clear status from 2\nPOKéMON?")) == 0);
    EXPECT_EQ(TryConditionCoachPartyChoice(CONDITION_COACH_CHOICE_CLEAR), CONDITION_COACH_RESULT_APPLIED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_NONE);
    EXPECT_EQ(GetMonData(&gPlayerParty[1], MON_DATA_STATUS), STATUS1_NONE);
    EXPECT_EQ(GetMonData(&gPlayerParty[2], MON_DATA_STATUS), STATUS1_NONE);
    EXPECT_EQ(gSpecialVar_0x8007, 2);
    EXPECT_EQ(gSpecialVar_0x800A, 1);

    EXPECT_EQ(ConditionCoach_TryPreviewParty(), CONDITION_COACH_RESULT_ALREADY_CLEAR);
    EXPECT_EQ(TryConditionCoachPartyChoice(CONDITION_COACH_CHOICE_CLEAR), CONDITION_COACH_RESULT_ALREADY_CLEAR);
}

TEST("Condition Coach whole-party application is atomic when the choice is locked")
{
    u32 status = STATUS1_POISON;

    ZeroPlayerPartyMons();
    AddConditionCoachTestMon(0, SPECIES_WOBBUFFET, 0);
    AddConditionCoachTestMon(1, SPECIES_MACHOP, 0);
    SetMonData(&gPlayerParty[1], MON_DATA_STATUS, &status);
    CalculatePlayerPartyCount();
    SetConditionCoachBadgeCount(1);

    EXPECT_EQ(TryConditionCoachPartyChoice(CONDITION_COACH_CHOICE_BURN), CONDITION_COACH_RESULT_LOCKED);
    EXPECT_EQ(gSpecialVar_0x8006, 2);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), STATUS1_NONE);
    EXPECT_EQ(GetMonData(&gPlayerParty[1], MON_DATA_STATUS), STATUS1_POISON);
}

TEST("Condition Coach reports when the party has no eligible targets")
{
    u32 isEgg = TRUE;
    u32 hp = 0;

    ZeroPlayerPartyMons();
    AddConditionCoachTestMon(0, SPECIES_PICHU, 0);
    AddConditionCoachTestMon(1, SPECIES_WOBBUFFET, 0);
    SetMonData(&gPlayerParty[0], MON_DATA_IS_EGG, &isEgg);
    SetMonData(&gPlayerParty[1], MON_DATA_HP, &hp);
    CalculatePlayerPartyCount();
    SetConditionCoachBadgeCount(NUM_BADGES);

    EXPECT_EQ(TryConditionCoachPartyChoice(CONDITION_COACH_CHOICE_POISON), CONDITION_COACH_RESULT_NO_ELIGIBLE);
    EXPECT_EQ(gSpecialVar_0x8007, 0);
    EXPECT_EQ(gSpecialVar_0x8008, 1);
    EXPECT_EQ(gSpecialVar_0x8009, 1);
}

TEST("Condition Coach whole-party feedback retains both ability benefits and warnings")
{
    u32 poisonHealSlot = 1;
    u32 toxicBoostSlot = 2;
    u32 item = ITEM_PECHA_BERRY;

    ASSUME(GetSpeciesAbility(SPECIES_BRELOOM, poisonHealSlot) == ABILITY_POISON_HEAL);
    ASSUME(GetSpeciesAbility(SPECIES_ZANGOOSE, toxicBoostSlot) == ABILITY_TOXIC_BOOST);

    ZeroPlayerPartyMons();
    AddConditionCoachTestMon(0, SPECIES_BRELOOM, poisonHealSlot);
    AddConditionCoachTestMon(1, SPECIES_ZANGOOSE, toxicBoostSlot);
    SetMonData(&gPlayerParty[0], MON_DATA_HELD_ITEM, &item);
    CalculatePlayerPartyCount();
    SetConditionCoachBadgeCount(NUM_BADGES);

    EXPECT_EQ(TryConditionCoachPartyChoice(CONDITION_COACH_CHOICE_POISON), CONDITION_COACH_RESULT_APPLIED);
    EXPECT(gSpecialVar_0x800B & CONDITION_COACH_SIGNAL_POISON_HEAL);
    EXPECT(gSpecialVar_0x800B & CONDITION_COACH_SIGNAL_TOXIC_BOOST);
    EXPECT(gSpecialVar_0x800B & CONDITION_COACH_SIGNAL_CURING_ITEM);
    EXPECT(StringCompare(gStringVar4, COMPOUND_STRING("Done. 2 POKéMON were poisoned.\pBreloom brings Poison Heal.\nToxic Boost is ready too.\lBreloom's held Berry may\nundo the setup.")) == 0);
}
