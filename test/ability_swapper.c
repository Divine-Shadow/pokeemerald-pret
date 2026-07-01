#include "global.h"
#include "event_data.h"
#include "item.h"
#include "pokemon.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "test/test.h"
#include "constants/abilities.h"
#include "constants/ability_swapper.h"
#include "constants/items.h"
#include "constants/party_menu.h"
#include "constants/species.h"

static void CreateAbilitySwapperTestMon(u16 species, u8 abilityNum)
{
    ClearBag();
    ZeroPlayerPartyMons();
    CreateMon(&gPlayerParty[0], species, 50, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
    SetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM, &abilityNum);
    CalculatePlayerPartyCount();
    gSpecialVar_0x8004 = 0;
    gSpecialVar_0x8005 = 0;
    gSpecialVar_0x8006 = 0;
}

static void ExpectStringVar1MatchesFirstPartyMonNickname(void)
{
    u8 nickname[POKEMON_NAME_LENGTH + 1];

    GetMonNickname(&gPlayerParty[0], nickname);
    EXPECT(StringCompare(gStringVar1, nickname) == 0);
}

TEST("Ability Swapper previews the alternate ordinary ability")
{
    CreateAbilitySwapperTestMon(SPECIES_BRELOOM, 0);
    ASSUME(GetSpeciesAbility(SPECIES_BRELOOM, 0) == ABILITY_EFFECT_SPORE);
    ASSUME(GetSpeciesAbility(SPECIES_BRELOOM, 1) == ABILITY_POISON_HEAL);

    EXPECT_EQ(AbilitySwapper_TryPreview(), ABILITY_SWAPPER_RESULT_READY);
    EXPECT(StringCompare(gStringVar2, gAbilitiesInfo[ABILITY_EFFECT_SPORE].name) == 0);
    EXPECT(StringCompare(gStringVar3, gAbilitiesInfo[ABILITY_POISON_HEAL].name) == 0);
    EXPECT_EQ(gSpecialVar_0x8005, 1);
    EXPECT_EQ(gSpecialVar_0x8006, ABILITY_POISON_HEAL);
}

TEST("Ability Swapper spends one Heart Scale and swaps to slot one")
{
    CreateAbilitySwapperTestMon(SPECIES_BRELOOM, 0);
    ASSUME(GetSpeciesAbility(SPECIES_BRELOOM, 0) == ABILITY_EFFECT_SPORE);
    ASSUME(GetSpeciesAbility(SPECIES_BRELOOM, 1) == ABILITY_POISON_HEAL);
    EXPECT(AddBagItem(ITEM_HEART_SCALE, 2));

    EXPECT_EQ(AbilitySwapper_TrySwap(), ABILITY_SWAPPER_RESULT_SWAPPED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM), 1);
    EXPECT_EQ(GetMonAbility(&gPlayerParty[0]), ABILITY_POISON_HEAL);
    EXPECT(CheckBagHasItem(ITEM_HEART_SCALE, 1));
    EXPECT(!CheckBagHasItem(ITEM_HEART_SCALE, 2));
}

TEST("Ability Swapper spends one Heart Scale and swaps back to slot zero")
{
    CreateAbilitySwapperTestMon(SPECIES_BRELOOM, 1);
    ASSUME(GetSpeciesAbility(SPECIES_BRELOOM, 0) == ABILITY_EFFECT_SPORE);
    ASSUME(GetSpeciesAbility(SPECIES_BRELOOM, 1) == ABILITY_POISON_HEAL);
    EXPECT(AddBagItem(ITEM_HEART_SCALE, 1));

    EXPECT_EQ(AbilitySwapper_TrySwap(), ABILITY_SWAPPER_RESULT_SWAPPED);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM), 0);
    EXPECT_EQ(GetMonAbility(&gPlayerParty[0]), ABILITY_EFFECT_SPORE);
    EXPECT(!CheckBagHasItem(ITEM_HEART_SCALE, 1));
}

TEST("Ability Swapper refuses without a Heart Scale and does not change ability")
{
    CreateAbilitySwapperTestMon(SPECIES_BRELOOM, 0);
    ASSUME(GetSpeciesAbility(SPECIES_BRELOOM, 1) == ABILITY_POISON_HEAL);

    EXPECT_EQ(AbilitySwapper_TrySwap(), ABILITY_SWAPPER_RESULT_NO_HEART_SCALE);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM), 0);
    EXPECT(!CheckBagHasItem(ITEM_HEART_SCALE, 1));
}

TEST("Ability Swapper refuses Pokemon with no second ordinary ability")
{
    CreateAbilitySwapperTestMon(SPECIES_WOBBUFFET, 0);
    ASSUME(GetSpeciesAbility(SPECIES_WOBBUFFET, 1) == ABILITY_NONE);
    EXPECT(AddBagItem(ITEM_HEART_SCALE, 1));

    StringCopy(gStringVar1, COMPOUND_STRING("STALE"));
    EXPECT_EQ(AbilitySwapper_TrySwap(), ABILITY_SWAPPER_RESULT_NO_ORDINARY_SWAP);
    ExpectStringVar1MatchesFirstPartyMonNickname();
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM), 0);
    EXPECT(CheckBagHasItem(ITEM_HEART_SCALE, 1));
}

TEST("Ability Swapper refuses duplicate ordinary ability slots")
{
    CreateAbilitySwapperTestMon(SPECIES_VIBRAVA, 0);
    ASSUME(GetSpeciesAbility(SPECIES_VIBRAVA, 0) != ABILITY_NONE);
    ASSUME(GetSpeciesAbility(SPECIES_VIBRAVA, 0) == GetSpeciesAbility(SPECIES_VIBRAVA, 1));
    EXPECT(AddBagItem(ITEM_HEART_SCALE, 1));

    EXPECT_EQ(AbilitySwapper_TrySwap(), ABILITY_SWAPPER_RESULT_NO_ORDINARY_SWAP);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM), 0);
    EXPECT(CheckBagHasItem(ITEM_HEART_SCALE, 1));
}

TEST("Ability Swapper refuses Eggs")
{
    u8 isEgg = TRUE;

    CreateAbilitySwapperTestMon(SPECIES_BRELOOM, 0);
    SetMonData(&gPlayerParty[0], MON_DATA_IS_EGG, &isEgg);
    EXPECT(AddBagItem(ITEM_HEART_SCALE, 1));

    EXPECT_EQ(AbilitySwapper_TrySwap(), ABILITY_SWAPPER_RESULT_EGG);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM), 0);
    EXPECT(CheckBagHasItem(ITEM_HEART_SCALE, 1));
}

TEST("Ability Swapper refuses Hidden Ability Pokemon without downgrading them")
{
    CreateAbilitySwapperTestMon(SPECIES_BRELOOM, 2);
    ASSUME(GetSpeciesAbility(SPECIES_BRELOOM, 2) == ABILITY_TECHNICIAN);
    EXPECT(AddBagItem(ITEM_HEART_SCALE, 1));

    StringCopy(gStringVar1, COMPOUND_STRING("STALE"));
    EXPECT_EQ(AbilitySwapper_TryPreview(), ABILITY_SWAPPER_RESULT_HIDDEN_ABILITY);
    ExpectStringVar1MatchesFirstPartyMonNickname();
    EXPECT_EQ(AbilitySwapper_TrySwap(), ABILITY_SWAPPER_RESULT_HIDDEN_ABILITY);
    EXPECT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM), 2);
    EXPECT_EQ(GetMonAbility(&gPlayerParty[0]), ABILITY_TECHNICIAN);
    EXPECT(CheckBagHasItem(ITEM_HEART_SCALE, 1));
}

TEST("Ability Swapper treats party cancellation as cancelled")
{
    CreateAbilitySwapperTestMon(SPECIES_BRELOOM, 0);
    gSpecialVar_0x8004 = PARTY_NOTHING_CHOSEN;

    EXPECT_EQ(AbilitySwapper_TryPreview(), ABILITY_SWAPPER_RESULT_CANCELLED);
}
