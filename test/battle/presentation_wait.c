#include "global.h"
#include "constants/hold_effects.h"
#include "battle_presentation.h"
#include "test/test.h"
#include "constants/battle.h"

static bool32 ShouldAdvance(
    u32 battleTypeFlags,
    u16 newKeys,
    bool32 controllerBusy,
    bool32 textPrinterActive,
    bool32 soundActive,
    bool32 messageActive,
    bool32 timeoutReached,
    bool32 headless)
{
    return ShouldAdvanceBattlePresentationWait(
        battleTypeFlags,
        newKeys,
        controllerBusy,
        textPrinterActive,
        soundActive,
        messageActive,
        timeoutReached,
        headless);
}

TEST("Battle presentation wait/A advances only after presentation completes")
{
    EXPECT(!ShouldAdvance(0, A_BUTTON, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE));
    EXPECT(!ShouldAdvance(0, A_BUTTON, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE));
    EXPECT(!ShouldAdvance(0, A_BUTTON, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE));
    EXPECT(ShouldAdvance(0, A_BUTTON, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE));
}

TEST("Battle presentation wait/Held or unrelated buttons do not advance")
{
    EXPECT(!ShouldAdvance(0, 0, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE));
    EXPECT(!ShouldAdvance(0, B_BUTTON, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE));
    EXPECT(!ShouldAdvance(0, R_BUTTON, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE));
}

TEST("Battle presentation wait/Expired timeout advances only after sound completes")
{
    EXPECT(!ShouldAdvance(0, 0, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE));
    EXPECT(ShouldAdvance(0, 0, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE));
    EXPECT(ShouldAdvance(0, 0, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE));
}

TEST("Battle presentation wait/Link and recorded battles ignore A")
{
    EXPECT(!ShouldAdvance(BATTLE_TYPE_LINK, A_BUTTON, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE));
    EXPECT(ShouldAdvance(BATTLE_TYPE_LINK, A_BUTTON, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE));
    EXPECT(!ShouldAdvance(BATTLE_TYPE_RECORDED, A_BUTTON, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE));
    EXPECT(ShouldAdvance(BATTLE_TYPE_RECORDED, A_BUTTON, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE));
    EXPECT(!ShouldAdvance(0, A_BUTTON, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE));
    EXPECT(ShouldAdvance(0, A_BUTTON, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE));
}
