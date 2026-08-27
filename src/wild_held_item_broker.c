#include "global.h"
#include "event_data.h"
#include "item.h"
#include "pokemon.h"
#include "script.h"
#include "shop.h"
#include "string_util.h"
#include "wild_held_item_broker.h"
#include "constants/abilities.h"
#include "constants/hold_effects.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"

static EWRAM_DATA u16 sWildHeldItemBrokerShopItems[ITEMS_COUNT + 1] = {0};
static EWRAM_DATA u16 sWildHeldItemBrokerShopItemCount = 0;

static void WildHeldItemBroker_BuildShopItems(enum WildHeldItemBrokerCategory category);
static bool8 WildHeldItemBroker_TryAppendShopItem(u16 item, enum WildHeldItemBrokerCategory category);
static bool8 WildHeldItemBroker_HasShopItem(u16 item);
static void WildHeldItemBroker_SortShopItems(enum WildHeldItemBrokerCategory category);
static bool32 WildHeldItemBroker_IsOneUseHold(u16 item);
static bool32 WildHeldItemBroker_IsEvolutionOrFormItem(u16 item);

bool32 WildHeldItemBroker_IsEligibleMove(u16 move)
{
    switch (move)
    {
    case MOVE_THIEF:
    case MOVE_COVET:
    case MOVE_TRICK:
    case MOVE_SWITCHEROO:
        return TRUE;
    default:
        return FALSE;
    }
}

bool32 WildHeldItemBroker_IsEligibleAbility(u16 ability)
{
    switch (ability)
    {
    case ABILITY_MAGICIAN:
    case ABILITY_PICKPOCKET:
        return TRUE;
    default:
        return FALSE;
    }
}

bool32 WildHeldItemBroker_IsEligibleMon(struct Pokemon *mon)
{
    u32 i;
    u16 species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG);

    if (species == SPECIES_NONE || species == SPECIES_EGG)
        return FALSE;

    if (WildHeldItemBroker_IsEligibleAbility(GetMonAbility(mon)))
        return TRUE;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (WildHeldItemBroker_IsEligibleMove(GetMonData(mon, MON_DATA_MOVE1 + i)))
            return TRUE;
    }

    return FALSE;
}

bool32 WildHeldItemBroker_IsItemAvailable(u16 item)
{
    if (item == ITEM_NONE || item >= ITEMS_COUNT)
        return FALSE;

    // Future progression and acquisition gates belong here, before category filtering.
    switch (item)
    {
    case ITEM_CHOICE_BAND:
    case ITEM_CHOICE_SPECS:
    case ITEM_CHOICE_SCARF:
    case ITEM_EVIOLITE:
    case ITEM_FOCUS_SASH:
    case ITEM_LEFTOVERS:
    case ITEM_LIFE_ORB:
    case ITEM_ASSAULT_VEST:
        return FALSE;
    default:
        return TRUE;
    }
}

u32 WildHeldItemBroker_GetItemCategoryMask(u16 item)
{
    u32 mask = 0;
    enum ItemSortType sortType;

    if (item == ITEM_NONE || item >= ITEMS_COUNT)
        return 0;

    sortType = gItemsInfo[item].sortType;
    if (GetItemHoldEffect(item) == HOLD_EFFECT_TYPE_POWER)
        mask |= 1u << WILD_HELD_ITEM_BROKER_CATEGORY_TYPE_BOOSTS;
    else if (WildHeldItemBroker_IsOneUseHold(item))
        mask |= 1u << WILD_HELD_ITEM_BROKER_CATEGORY_ONE_USE_HOLDS;
    else if (GetItemHoldEffect(item) != HOLD_EFFECT_NONE)
        mask |= 1u << WILD_HELD_ITEM_BROKER_CATEGORY_HELD_GEAR;

    if (WildHeldItemBroker_IsEvolutionOrFormItem(item))
        mask |= 1u << WILD_HELD_ITEM_BROKER_CATEGORY_EVOLUTION_FORM;

    if (GetItemHoldEffect(item) == HOLD_EFFECT_NONE
     && !(mask & (1u << WILD_HELD_ITEM_BROKER_CATEGORY_EVOLUTION_FORM)))
    {
        if (item != ITEM_HONEY
         && (sortType == ITEM_TYPE_HEALTH_RECOVERY
          || sortType == ITEM_TYPE_STATUS_RECOVERY
          || sortType == ITEM_TYPE_PP_RECOVERY))
            mask |= 1u << WILD_HELD_ITEM_BROKER_CATEGORY_MEDICINE;
        else
            mask |= 1u << WILD_HELD_ITEM_BROKER_CATEGORY_RESOURCES;
    }

    // Keep future wild drops visible even when their item metadata has no useful category yet.
    if (mask == 0)
        mask = 1u << WILD_HELD_ITEM_BROKER_CATEGORY_RESOURCES;

    return mask;
}

const u16 *WildHeldItemBroker_GetShopItems(enum WildHeldItemBrokerCategory category)
{
    WildHeldItemBroker_BuildShopItems(category);
    return sWildHeldItemBrokerShopItems;
}

u16 WildHeldItemBroker_GetShopItemCount(enum WildHeldItemBrokerCategory category)
{
    WildHeldItemBroker_BuildShopItems(category);
    return sWildHeldItemBrokerShopItemCount;
}

bool8 Script_IsWildHeldItemBrokerEligiblePartyMon(void)
{
    if (gSpecialVar_0x8004 >= PARTY_SIZE)
        return FALSE;

    return WildHeldItemBroker_IsEligibleMon(&gPlayerParty[gSpecialVar_0x8004]);
}

void OpenWildHeldItemBrokerShop(void)
{
    enum WildHeldItemBrokerCategory category = gSpecialVar_0x8004;

    if (category > WILD_HELD_ITEM_BROKER_CATEGORY_ALL)
        category = WILD_HELD_ITEM_BROKER_CATEGORY_ALL;

    CreateDirectFreePokemartMenu(WildHeldItemBroker_GetShopItems(category));
    ScriptContext_Stop();
}

static void WildHeldItemBroker_BuildShopItems(enum WildHeldItemBrokerCategory category)
{
    u16 species;

    sWildHeldItemBrokerShopItemCount = 0;

    for (species = SPECIES_NONE + 1; species < NUM_SPECIES; species++)
    {
        WildHeldItemBroker_TryAppendShopItem(gSpeciesInfo[species].itemCommon, category);
        WildHeldItemBroker_TryAppendShopItem(gSpeciesInfo[species].itemRare, category);
    }

    WildHeldItemBroker_SortShopItems(category);
    sWildHeldItemBrokerShopItems[sWildHeldItemBrokerShopItemCount] = ITEM_NONE;
}

static bool8 WildHeldItemBroker_TryAppendShopItem(u16 item, enum WildHeldItemBrokerCategory category)
{
    if (!WildHeldItemBroker_IsItemAvailable(item)
     || category >= WILD_HELD_ITEM_BROKER_CATEGORY_EXIT
     || (category != WILD_HELD_ITEM_BROKER_CATEGORY_ALL
      && !(WildHeldItemBroker_GetItemCategoryMask(item) & (1u << category)))
     || WildHeldItemBroker_HasShopItem(item))
        return FALSE;

    sWildHeldItemBrokerShopItems[sWildHeldItemBrokerShopItemCount++] = item;
    return TRUE;
}

static void WildHeldItemBroker_SortShopItems(enum WildHeldItemBrokerCategory category)
{
    u16 i, j;

    for (i = 1; i < sWildHeldItemBrokerShopItemCount; i++)
    {
        u16 item = sWildHeldItemBrokerShopItems[i];

        for (j = i; j > 0; j--)
        {
            u16 previous = sWildHeldItemBrokerShopItems[j - 1];
            bool32 itemComesFirst = StringCompare(GetItemName(item), GetItemName(previous)) < 0;

            if (category == WILD_HELD_ITEM_BROKER_CATEGORY_RESOURCES)
            {
                if (item == ITEM_HEART_SCALE)
                    itemComesFirst = TRUE;
                else if (previous == ITEM_HEART_SCALE)
                    itemComesFirst = FALSE;
            }

            if (!itemComesFirst)
                break;

            sWildHeldItemBrokerShopItems[j] = previous;
        }

        sWildHeldItemBrokerShopItems[j] = item;
    }
}

static bool32 WildHeldItemBroker_IsOneUseHold(u16 item)
{
    if (GetItemPocket(item) == POCKET_BERRIES)
        return TRUE;

    switch (GetItemHoldEffect(item))
    {
    case HOLD_EFFECT_RESTORE_HP:
    case HOLD_EFFECT_CURE_PAR:
    case HOLD_EFFECT_CURE_SLP:
    case HOLD_EFFECT_CURE_PSN:
    case HOLD_EFFECT_CURE_BRN:
    case HOLD_EFFECT_CURE_FRZ:
    case HOLD_EFFECT_RESTORE_PP:
    case HOLD_EFFECT_CURE_CONFUSION:
    case HOLD_EFFECT_CURE_STATUS:
    case HOLD_EFFECT_CONFUSE_SPICY:
    case HOLD_EFFECT_CONFUSE_DRY:
    case HOLD_EFFECT_CONFUSE_SWEET:
    case HOLD_EFFECT_CONFUSE_BITTER:
    case HOLD_EFFECT_CONFUSE_SOUR:
    case HOLD_EFFECT_ATTACK_UP:
    case HOLD_EFFECT_DEFENSE_UP:
    case HOLD_EFFECT_SPEED_UP:
    case HOLD_EFFECT_SP_ATTACK_UP:
    case HOLD_EFFECT_SP_DEFENSE_UP:
    case HOLD_EFFECT_CRITICAL_UP:
    case HOLD_EFFECT_RANDOM_STAT_UP:
    case HOLD_EFFECT_WHITE_HERB:
    case HOLD_EFFECT_MENTAL_HERB:
    case HOLD_EFFECT_FOCUS_SASH:
    case HOLD_EFFECT_POWER_HERB:
    case HOLD_EFFECT_ENIGMA_BERRY:
    case HOLD_EFFECT_RESIST_BERRY:
    case HOLD_EFFECT_RESTORE_PCT_HP:
    case HOLD_EFFECT_MICLE_BERRY:
    case HOLD_EFFECT_CUSTAP_BERRY:
    case HOLD_EFFECT_JABOCA_BERRY:
    case HOLD_EFFECT_ROWAP_BERRY:
    case HOLD_EFFECT_KEE_BERRY:
    case HOLD_EFFECT_MARANGA_BERRY:
    case HOLD_EFFECT_GEMS:
    case HOLD_EFFECT_AIR_BALLOON:
    case HOLD_EFFECT_RED_CARD:
    case HOLD_EFFECT_EJECT_BUTTON:
    case HOLD_EFFECT_ABSORB_BULB:
    case HOLD_EFFECT_CELL_BATTERY:
    case HOLD_EFFECT_LUMINOUS_MOSS:
    case HOLD_EFFECT_SNOWBALL:
    case HOLD_EFFECT_WEAKNESS_POLICY:
    case HOLD_EFFECT_SEEDS:
    case HOLD_EFFECT_ADRENALINE_ORB:
    case HOLD_EFFECT_EJECT_PACK:
    case HOLD_EFFECT_ROOM_SERVICE:
    case HOLD_EFFECT_BLUNDER_POLICY:
    case HOLD_EFFECT_THROAT_SPRAY:
    case HOLD_EFFECT_MIRROR_HERB:
    case HOLD_EFFECT_BOOSTER_ENERGY:
    case HOLD_EFFECT_BERSERK_GENE:
        return TRUE;
    default:
        return FALSE;
    }
}

static bool32 WildHeldItemBroker_IsEvolutionOrFormItem(u16 item)
{
    switch (gItemsInfo[item].sortType)
    {
    case ITEM_TYPE_EVOLUTION_STONE:
    case ITEM_TYPE_EVOLUTION_ITEM:
    case ITEM_TYPE_NECTAR:
        return TRUE;
    default:
        break;
    }

    switch (item)
    {
    case ITEM_METAL_COAT:
    case ITEM_KINGS_ROCK:
    case ITEM_DEEP_SEA_SCALE:
    case ITEM_DEEP_SEA_TOOTH:
    case ITEM_RAZOR_CLAW:
    case ITEM_RAZOR_FANG:
        return TRUE;
    default:
        return FALSE;
    }
}

static bool8 WildHeldItemBroker_HasShopItem(u16 item)
{
    u16 i;

    for (i = 0; i < sWildHeldItemBrokerShopItemCount; i++)
    {
        if (sWildHeldItemBrokerShopItems[i] == item)
            return TRUE;
    }

    return FALSE;
}
