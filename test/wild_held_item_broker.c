#include "global.h"
#include "item.h"
#include "pokemon.h"
#include "string_util.h"
#include "test/test.h"
#include "wild_held_item_broker.h"
#include "constants/abilities.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"

static void ClearMonMoves(struct Pokemon *mon)
{
    u32 i;
    u16 move = MOVE_NONE;

    for (i = 0; i < MAX_MON_MOVES; i++)
        SetMonData(mon, MON_DATA_MOVE1 + i, &move);
}

static bool32 ItemListContains(const u16 *items, u16 item)
{
    u16 i;

    for (i = 0; items[i] != ITEM_NONE; i++)
    {
        if (items[i] == item)
            return TRUE;
    }

    return FALSE;
}

static bool32 ShopItemsContain(enum WildHeldItemBrokerCategory category, u16 item)
{
    return ItemListContains(WildHeldItemBroker_GetShopItems(category), item);
}

TEST("Wild held item broker accepts Pokemon with item-stealing moves")
{
    u16 move;
    struct Pokemon mon;

    PARAMETRIZE { move = MOVE_THIEF; }
    PARAMETRIZE { move = MOVE_COVET; }
    PARAMETRIZE { move = MOVE_TRICK; }
    PARAMETRIZE { move = MOVE_SWITCHEROO; }

    CreateMon(&mon, SPECIES_WOBBUFFET, 30, 0, FALSE, 0, OT_ID_PRESET, 0);
    ClearMonMoves(&mon);
    SetMonData(&mon, MON_DATA_MOVE3, &move);

    EXPECT(WildHeldItemBroker_IsEligibleMon(&mon));
}

TEST("Wild held item broker accepts Pokemon with item-stealing abilities")
{
    u16 species;
    u16 ability;
    u8 abilityNum;
    struct Pokemon mon;

    PARAMETRIZE { species = SPECIES_FENNEKIN; ability = ABILITY_MAGICIAN; abilityNum = 2; }
    PARAMETRIZE { species = SPECIES_SNEASEL; ability = ABILITY_PICKPOCKET; abilityNum = 2; }

    CreateMon(&mon, species, 30, 0, FALSE, 0, OT_ID_PRESET, 0);
    ClearMonMoves(&mon);
    SetMonData(&mon, MON_DATA_ABILITY_NUM, &abilityNum);

    ASSUME(GetMonAbility(&mon) == ability);
    EXPECT(WildHeldItemBroker_IsEligibleMon(&mon));
}

TEST("Wild held item broker rejects scouting or non-acquiring options")
{
    u16 species;
    u16 move;
    u16 ability;
    u8 abilityNum;
    struct Pokemon mon;

    PARAMETRIZE { species = SPECIES_SENTRET; move = MOVE_NONE; ability = ABILITY_FRISK; abilityNum = 2; }
    PARAMETRIZE { species = SPECIES_ZIGZAGOON; move = MOVE_NONE; ability = ABILITY_PICKUP; abilityNum = 0; }
    PARAMETRIZE { species = SPECIES_WOBBUFFET; move = MOVE_KNOCK_OFF; ability = ABILITY_SHADOW_TAG; abilityNum = 0; }

    CreateMon(&mon, species, 30, 0, FALSE, 0, OT_ID_PRESET, 0);
    ClearMonMoves(&mon);
    SetMonData(&mon, MON_DATA_MOVE1, &move);
    SetMonData(&mon, MON_DATA_ABILITY_NUM, &abilityNum);

    ASSUME(GetMonAbility(&mon) == ability);
    EXPECT(!WildHeldItemBroker_IsEligibleMon(&mon));
}

TEST("Wild held item broker rejects eggs")
{
    u8 isEgg = TRUE;
    u16 move = MOVE_THIEF;
    struct Pokemon mon;

    CreateMon(&mon, SPECIES_WOBBUFFET, 30, 0, FALSE, 0, OT_ID_PRESET, 0);
    ClearMonMoves(&mon);
    SetMonData(&mon, MON_DATA_MOVE1, &move);
    SetMonData(&mon, MON_DATA_IS_EGG, &isEgg);

    EXPECT(!WildHeldItemBroker_IsEligibleMon(&mon));
}

TEST("Wild held item broker shop contains distinct wild held items")
{
    const u16 *items = WildHeldItemBroker_GetShopItems(WILD_HELD_ITEM_BROKER_CATEGORY_ALL);
    u16 count = WildHeldItemBroker_GetShopItemCount(WILD_HELD_ITEM_BROKER_CATEGORY_ALL);
    u16 i, j;

    ASSUME(gSpeciesInfo[SPECIES_PIKACHU].itemRare == ITEM_LIGHT_BALL);
    ASSUME(gSpeciesInfo[SPECIES_ABRA].itemRare == ITEM_TWISTED_SPOON);

    EXPECT_GT(count, 0);
    EXPECT_EQ(items[count], ITEM_NONE);
    EXPECT(ShopItemsContain(WILD_HELD_ITEM_BROKER_CATEGORY_ALL, ITEM_LIGHT_BALL));
    EXPECT(ShopItemsContain(WILD_HELD_ITEM_BROKER_CATEGORY_ALL, ITEM_TWISTED_SPOON));

    for (i = 0; i < count; i++)
    {
        EXPECT_NE(items[i], ITEM_NONE);
        EXPECT_LT(items[i], ITEMS_COUNT);

        for (j = i + 1; j < count; j++)
            EXPECT_NE(items[i], items[j]);
    }
}

TEST("Wild held item broker excludes strict competitive items")
{
    ASSUME(gSpeciesInfo[SPECIES_SNORLAX].itemCommon == ITEM_LEFTOVERS);

    EXPECT(!ShopItemsContain(WILD_HELD_ITEM_BROKER_CATEGORY_ALL, ITEM_CHOICE_BAND));
    EXPECT(!ShopItemsContain(WILD_HELD_ITEM_BROKER_CATEGORY_ALL, ITEM_CHOICE_SPECS));
    EXPECT(!ShopItemsContain(WILD_HELD_ITEM_BROKER_CATEGORY_ALL, ITEM_CHOICE_SCARF));
    EXPECT(!ShopItemsContain(WILD_HELD_ITEM_BROKER_CATEGORY_ALL, ITEM_EVIOLITE));
    EXPECT(!ShopItemsContain(WILD_HELD_ITEM_BROKER_CATEGORY_ALL, ITEM_FOCUS_SASH));
    EXPECT(!ShopItemsContain(WILD_HELD_ITEM_BROKER_CATEGORY_ALL, ITEM_LEFTOVERS));
    EXPECT(!ShopItemsContain(WILD_HELD_ITEM_BROKER_CATEGORY_ALL, ITEM_LIFE_ORB));
    EXPECT(!ShopItemsContain(WILD_HELD_ITEM_BROKER_CATEGORY_ALL, ITEM_ASSAULT_VEST));

    EXPECT(!WildHeldItemBroker_IsItemAvailable(ITEM_CHOICE_BAND));
    EXPECT(!WildHeldItemBroker_IsItemAvailable(ITEM_CHOICE_SPECS));
    EXPECT(!WildHeldItemBroker_IsItemAvailable(ITEM_CHOICE_SCARF));
    EXPECT(!WildHeldItemBroker_IsItemAvailable(ITEM_EVIOLITE));
    EXPECT(!WildHeldItemBroker_IsItemAvailable(ITEM_FOCUS_SASH));
    EXPECT(!WildHeldItemBroker_IsItemAvailable(ITEM_LEFTOVERS));
    EXPECT(!WildHeldItemBroker_IsItemAvailable(ITEM_LIFE_ORB));
    EXPECT(!WildHeldItemBroker_IsItemAvailable(ITEM_ASSAULT_VEST));
}

TEST("Wild held item broker assigns semantic and overlapping category masks")
{
    u16 item;
    u32 expectedMask;

    PARAMETRIZE { item = ITEM_LIGHT_BALL;      expectedMask = WILD_HELD_ITEM_BROKER_MASK_HELD_GEAR; }
    PARAMETRIZE { item = ITEM_BOOSTER_ENERGY;  expectedMask = WILD_HELD_ITEM_BROKER_MASK_ONE_USE_HOLDS; }
    PARAMETRIZE { item = ITEM_SILK_SCARF;      expectedMask = WILD_HELD_ITEM_BROKER_MASK_TYPE_BOOSTS; }
    PARAMETRIZE { item = ITEM_MOON_STONE;      expectedMask = WILD_HELD_ITEM_BROKER_MASK_EVOLUTION_FORM; }
    PARAMETRIZE { item = ITEM_POTION;          expectedMask = WILD_HELD_ITEM_BROKER_MASK_MEDICINE; }
    PARAMETRIZE { item = ITEM_HEART_SCALE;     expectedMask = WILD_HELD_ITEM_BROKER_MASK_RESOURCES; }
    PARAMETRIZE { item = ITEM_METAL_COAT;      expectedMask = WILD_HELD_ITEM_BROKER_MASK_TYPE_BOOSTS | WILD_HELD_ITEM_BROKER_MASK_EVOLUTION_FORM; }
    PARAMETRIZE { item = ITEM_KINGS_ROCK;      expectedMask = WILD_HELD_ITEM_BROKER_MASK_HELD_GEAR | WILD_HELD_ITEM_BROKER_MASK_EVOLUTION_FORM; }
    PARAMETRIZE { item = ITEM_EVERSTONE;       expectedMask = WILD_HELD_ITEM_BROKER_MASK_HELD_GEAR | WILD_HELD_ITEM_BROKER_MASK_EVOLUTION_FORM; }
    PARAMETRIZE { item = ITEM_DEEP_SEA_TOOTH;  expectedMask = WILD_HELD_ITEM_BROKER_MASK_HELD_GEAR | WILD_HELD_ITEM_BROKER_MASK_EVOLUTION_FORM; }
    PARAMETRIZE { item = ITEM_ELECTIRIZER;     expectedMask = WILD_HELD_ITEM_BROKER_MASK_EVOLUTION_FORM; }
    PARAMETRIZE { item = ITEM_RED_NECTAR;      expectedMask = WILD_HELD_ITEM_BROKER_MASK_EVOLUTION_FORM; }
    PARAMETRIZE { item = ITEM_BERRY_JUICE;     expectedMask = WILD_HELD_ITEM_BROKER_MASK_ONE_USE_HOLDS; }
    PARAMETRIZE { item = ITEM_HONEY;           expectedMask = WILD_HELD_ITEM_BROKER_MASK_RESOURCES; }
    PARAMETRIZE { item = ITEM_SACRED_ASH;      expectedMask = WILD_HELD_ITEM_BROKER_MASK_MEDICINE; }
    PARAMETRIZE { item = ITEM_LEFTOVERS;       expectedMask = WILD_HELD_ITEM_BROKER_MASK_HELD_GEAR; }

    EXPECT_EQ(WildHeldItemBroker_GetItemCategoryMask(item), expectedMask);
}

TEST("Wild held item broker rejects invalid items before availability or categorization")
{
    EXPECT_EQ(WildHeldItemBroker_GetItemCategoryMask(ITEM_NONE), 0);
    EXPECT_EQ(WildHeldItemBroker_GetItemCategoryMask(ITEMS_COUNT), 0);
    EXPECT(!WildHeldItemBroker_IsItemAvailable(ITEM_NONE));
    EXPECT(!WildHeldItemBroker_IsItemAvailable(ITEMS_COUNT));
}

TEST("Wild held item broker category views are complete sorted and internally consistent")
{
    bool8 inAll[ITEMS_COUNT] = {FALSE};
    u8 viewMasks[ITEMS_COUNT] = {0};
    const u16 *items;
    enum WildHeldItemBrokerCategory category;
    u16 count;
    u16 i, j, species;

    items = WildHeldItemBroker_GetShopItems(WILD_HELD_ITEM_BROKER_CATEGORY_ALL);
    count = WildHeldItemBroker_GetShopItemCount(WILD_HELD_ITEM_BROKER_CATEGORY_ALL);
    for (i = 0; i < count; i++)
        inAll[items[i]] = TRUE;

    for (category = WILD_HELD_ITEM_BROKER_CATEGORY_HELD_GEAR;
         category < WILD_HELD_ITEM_BROKER_CATEGORY_ALL;
         category++)
    {
        items = WildHeldItemBroker_GetShopItems(category);
        count = WildHeldItemBroker_GetShopItemCount(category);
        EXPECT_GT(count, 0);
        EXPECT_EQ(items[count], ITEM_NONE);

        for (i = 0; i < count; i++)
        {
            EXPECT(WildHeldItemBroker_IsItemAvailable(items[i]));
            EXPECT(inAll[items[i]]);
            EXPECT(WildHeldItemBroker_GetItemCategoryMask(items[i]) & (1u << category));
            viewMasks[items[i]] |= 1u << category;

            if (i > 0 && items[i - 1] != ITEM_HEART_SCALE)
                EXPECT_LE(StringCompare(GetItemName(items[i - 1]), GetItemName(items[i])), 0);

            for (j = i + 1; j < count; j++)
                EXPECT_NE(items[i], items[j]);
        }
    }

    items = WildHeldItemBroker_GetShopItems(WILD_HELD_ITEM_BROKER_CATEGORY_ALL);
    count = WildHeldItemBroker_GetShopItemCount(WILD_HELD_ITEM_BROKER_CATEGORY_ALL);
    for (i = 0; i < count; i++)
    {
        EXPECT_EQ(viewMasks[items[i]], WildHeldItemBroker_GetItemCategoryMask(items[i]));
        if (i > 0)
            EXPECT_LE(StringCompare(GetItemName(items[i - 1]), GetItemName(items[i])), 0);
    }

    for (species = SPECIES_NONE + 1; species < NUM_SPECIES; species++)
    {
        u16 common = gSpeciesInfo[species].itemCommon;
        u16 rare = gSpeciesInfo[species].itemRare;

        if (WildHeldItemBroker_IsItemAvailable(common))
            EXPECT(inAll[common]);
        if (WildHeldItemBroker_IsItemAvailable(rare))
            EXPECT(inAll[rare]);
    }
}

TEST("Wild held item broker pins Heart Scale first in Resources")
{
    const u16 *items = WildHeldItemBroker_GetShopItems(WILD_HELD_ITEM_BROKER_CATEGORY_RESOURCES);

    EXPECT_EQ(items[0], ITEM_HEART_SCALE);
}
