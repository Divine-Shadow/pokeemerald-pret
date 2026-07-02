#include "global.h"
#include "boundary_charm.h"
#include "event_data.h"
#include "highlander_charm.h"
#include "item.h"
#include "overworld.h"
#include "pokemon.h"
#include "script.h"
#include "wild_encounter.h"
#include "test/test.h"
#include "constants/item.h"
#include "constants/items.h"
#include "constants/layouts.h"
#include "constants/region_map_sections.h"
#include "constants/species.h"
#include "constants/vars.h"

static void ResetBoundaryCharmTestState(void)
{
    ClearBag();
    SetBoundaryCharmActive(FALSE);
    ClearBoundaryCharmClaims();
    ClearBoundaryCharmEncounterSuppressed();
    SetHighlanderCharmActive(FALSE);
    VarSet(VAR_REPEL_STEP_COUNT, 0);
}

static const struct WildPokemonInfo *GetBoundaryCharmWaterMonsInfo(void)
{
    static const struct WildPokemon wildMons[WATER_WILD_COUNT] =
    {
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
    };
    static const struct WildPokemonInfo wildMonInfo =
    {
        .encounterRate = 100,
        .wildPokemon = wildMons,
    };

    return &wildMonInfo;
}

TEST("Extinction Charm uses a saved toggle instead of bag presence")
{
    ResetBoundaryCharmTestState();
    EXPECT(!IsBoundaryCharmActive());

    EXPECT(AddBagItem(ITEM_BOUNDARY_CHARM, 1));
    EXPECT(!IsBoundaryCharmActive());

    SetBoundaryCharmActive(TRUE);
    EXPECT(IsBoundaryCharmActive());
    EXPECT(gSaveBlock3Ptr->boundaryCharmActive);

    ClearBag();
    EXPECT(IsBoundaryCharmActive());

    ToggleBoundaryCharmActive();
    EXPECT(!IsBoundaryCharmActive());
    EXPECT(!gSaveBlock3Ptr->boundaryCharmActive);
}

TEST("Extinction Charm records captured Pokemon met locations")
{
    struct Pokemon mon;
    u8 mapSec = MAPSEC_ROUTE_102;

    ResetBoundaryCharmTestState();
    CreateMon(&mon, SPECIES_RATTATA, 5, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
    SetMonData(&mon, MON_DATA_MET_LOCATION, &mapSec);

    ClaimBoundaryCharmLocationForMon(&mon);
    EXPECT(IsBoundaryCharmLocationClaimed(MAPSEC_ROUTE_102));
    EXPECT(!IsBoundaryCharmLocationClaimed(MAPSEC_ROUTE_103));
}

TEST("Extinction Charm records static captures after they are caught")
{
    struct Pokemon mon;
    u8 mapSec = MAPSEC_CAVE_OF_ORIGIN;

    ResetBoundaryCharmTestState();
    CreateMon(&mon, SPECIES_KYOGRE, 45, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
    SetMonData(&mon, MON_DATA_MET_LOCATION, &mapSec);

    ClaimBoundaryCharmLocationForMon(&mon);
    EXPECT(IsBoundaryCharmLocationClaimed(MAPSEC_CAVE_OF_ORIGIN));
}

TEST("Mirage Island reports a separate Route 130 location on its active island layout")
{
    EXPECT_EQ(Test_GetRoute130RegionMapSectionId(LAYOUT_ROUTE130, 44, 7), MAPSEC_ROUTE_130);
    EXPECT_EQ(Test_GetRoute130RegionMapSectionId(LAYOUT_ROUTE130_MIRAGE_ISLAND, 44, 7), MAPSEC_MIRAGE_ISLAND);
    EXPECT_EQ(Test_GetRoute130RegionMapSectionId(LAYOUT_ROUTE130_MIRAGE_ISLAND, 10, 20), MAPSEC_ROUTE_130);
    EXPECT_EQ(Test_GetRoute130RegionMapSectionId(LAYOUT_ROUTE130_MIRAGE_ISLAND, 34, 1), MAPSEC_ROUTE_130);
}

TEST("Extinction Charm keeps Route 130 and Mirage Island claims separate")
{
    u8 mirageMapSec = Test_GetRoute130RegionMapSectionId(LAYOUT_ROUTE130_MIRAGE_ISLAND, 44, 7);

    ResetBoundaryCharmTestState();
    SetBoundaryCharmActive(TRUE);
    ClaimBoundaryCharmLocation(MAPSEC_ROUTE_130);

    EXPECT_EQ(mirageMapSec, MAPSEC_MIRAGE_ISLAND);
    EXPECT(Test_TryGenerateBoundaryWildMonAtMapSec(GetBoundaryCharmWaterMonsInfo(), WILD_AREA_WATER, mirageMapSec));
    EXPECT(!WasBoundaryCharmEncounterSuppressed());

    ClaimBoundaryCharmLocation(mirageMapSec);
    EXPECT(!Test_TryGenerateBoundaryWildMonAtMapSec(GetBoundaryCharmWaterMonsInfo(), WILD_AREA_WATER, mirageMapSec));
    EXPECT(WasBoundaryCharmEncounterSuppressed());
}

TEST("Extinction Charm disabled does not suppress claimed normal encounters")
{
    ResetBoundaryCharmTestState();
    ClaimBoundaryCharmLocation(MAPSEC_ROUTE_102);

    EXPECT(Test_TryGenerateBoundaryWildMonAtMapSec(GetBoundaryCharmWaterMonsInfo(), WILD_AREA_WATER, MAPSEC_ROUTE_102));
    EXPECT(!WasBoundaryCharmEncounterSuppressed());
}

TEST("Extinction Charm suppresses claimed normal encounter areas")
{
    static const struct WildPokemon landMons[LAND_WILD_COUNT] =
    {
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
        { 5, 5, SPECIES_RATTATA },
    };
    static const struct WildPokemon rockMons[ROCK_WILD_COUNT] =
    {
        { 5, 5, SPECIES_GEODUDE },
        { 5, 5, SPECIES_GEODUDE },
        { 5, 5, SPECIES_GEODUDE },
        { 5, 5, SPECIES_GEODUDE },
        { 5, 5, SPECIES_GEODUDE },
    };
    static const struct WildPokemon fishMons[FISH_WILD_COUNT] =
    {
        { 5, 5, SPECIES_MAGIKARP },
        { 5, 5, SPECIES_GOLDEEN },
    };
    static const struct WildPokemonInfo landInfo =
    {
        .encounterRate = 100,
        .wildPokemon = landMons,
    };
    static const struct WildPokemonInfo rockInfo =
    {
        .encounterRate = 100,
        .wildPokemon = rockMons,
    };
    static const struct WildPokemonInfo fishInfo =
    {
        .encounterRate = 100,
        .wildPokemon = fishMons,
    };

    ResetBoundaryCharmTestState();
    SetBoundaryCharmActive(TRUE);
    ClaimBoundaryCharmLocation(MAPSEC_ROUTE_102);

    EXPECT(!Test_TryGenerateBoundaryWildMonAtMapSec(&landInfo, WILD_AREA_LAND, MAPSEC_ROUTE_102));
    EXPECT(WasBoundaryCharmEncounterSuppressed());
    EXPECT(!Test_TryGenerateBoundaryWildMonAtMapSec(GetBoundaryCharmWaterMonsInfo(), WILD_AREA_WATER, MAPSEC_ROUTE_102));
    EXPECT(WasBoundaryCharmEncounterSuppressed());
    EXPECT(!Test_TryGenerateBoundaryWildMonAtMapSec(&rockInfo, WILD_AREA_ROCKS, MAPSEC_ROUTE_102));
    EXPECT(WasBoundaryCharmEncounterSuppressed());
    EXPECT(!Test_TryGenerateBoundaryFishingWildMonAtMapSec(&fishInfo, OLD_ROD, MAPSEC_ROUTE_102));
    EXPECT(WasBoundaryCharmEncounterSuppressed());
}

TEST("Extinction Charm passive encounters suppress silently but deliberate checks give feedback")
{
    ResetBoundaryCharmTestState();
    ScriptContext_Stop();
    SetBoundaryCharmActive(TRUE);
    ClaimBoundaryCharmLocation(MAPSEC_ROUTE_102);

    EXPECT(!Test_TryGenerateBoundaryWildMonAtMapSec(GetBoundaryCharmWaterMonsInfo(), WILD_AREA_WATER, MAPSEC_ROUTE_102));
    EXPECT(WasBoundaryCharmEncounterSuppressed());
    EXPECT(!Test_TryStartWildEncounterFailureFeedback(FALSE));
    EXPECT(WasBoundaryCharmEncounterSuppressed());
    EXPECT(!ScriptContext_IsEnabled());

    EXPECT(Test_TryStartWildEncounterFailureFeedback(TRUE));
    EXPECT(!WasBoundaryCharmEncounterSuppressed());
    EXPECT(ScriptContext_IsEnabled());
    ScriptContext_Stop();
}

TEST("Extinction Charm allows unclaimed normal encounter areas")
{
    ResetBoundaryCharmTestState();
    SetBoundaryCharmActive(TRUE);
    ClaimBoundaryCharmLocation(MAPSEC_ROUTE_102);

    EXPECT(Test_TryGenerateBoundaryWildMonAtMapSec(GetBoundaryCharmWaterMonsInfo(), WILD_AREA_WATER, MAPSEC_ROUTE_103));
    EXPECT(!WasBoundaryCharmEncounterSuppressed());
}

TEST("Extinction Charm uses collapsed caught-at map sections")
{
    ResetBoundaryCharmTestState();
    SetBoundaryCharmActive(TRUE);
    ClaimBoundaryCharmLocation(MAPSEC_ROUTE_110);

    EXPECT(IsBoundaryCharmLocationClaimed(MAPSEC_ROUTE_110));
    EXPECT(ShouldSuppressBoundaryCharmEncounterAt(MAPSEC_ROUTE_110));
    EXPECT(!ShouldSuppressBoundaryCharmEncounterAt(MAPSEC_ROUTE_111));
}

TEST("Extinction Charm does not affect unflagged special encounter generation")
{
    ResetBoundaryCharmTestState();
    SetBoundaryCharmActive(TRUE);
    ClaimBoundaryCharmLocation(MAPSEC_ROUTE_102);

    EXPECT(Test_TryGenerateUnfilteredWildMonAtMapSec(GetBoundaryCharmWaterMonsInfo(), WILD_AREA_WATER, MAPSEC_ROUTE_102));
    EXPECT(!WasBoundaryCharmEncounterSuppressed());
}
