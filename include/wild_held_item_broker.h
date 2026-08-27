#ifndef GUARD_WILD_HELD_ITEM_BROKER_H
#define GUARD_WILD_HELD_ITEM_BROKER_H

#include "global.h"
#include "constants/wild_held_item_broker.h"

struct Pokemon;

bool32 WildHeldItemBroker_IsEligibleMove(u16 move);
bool32 WildHeldItemBroker_IsEligibleAbility(u16 ability);
bool32 WildHeldItemBroker_IsEligibleMon(struct Pokemon *mon);
bool32 WildHeldItemBroker_IsItemAvailable(u16 item);
u32 WildHeldItemBroker_GetItemCategoryMask(u16 item);
const u16 *WildHeldItemBroker_GetShopItems(enum WildHeldItemBrokerCategory category);
u16 WildHeldItemBroker_GetShopItemCount(enum WildHeldItemBrokerCategory category);
bool8 Script_IsWildHeldItemBrokerEligiblePartyMon(void);
void OpenWildHeldItemBrokerShop(void);

#endif // GUARD_WILD_HELD_ITEM_BROKER_H
