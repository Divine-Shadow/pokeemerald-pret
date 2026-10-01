#include "global.h"
#include "battle.h"
#include "battle_presentation.h"
#include "main.h"
#include "text.h"
#include "text_reading.h"
#include "constants/battle.h"
#include "test/test.h"

static EWRAM_DATA struct TextReadingState sBattleTextReading = {0};
#if TESTING
static EWRAM_DATA bool8 sVisibleReadingForTesting = FALSE;

void SetBattleTextReadingVisibleForTesting(bool32 visible)
{
    ResetBattleTextReading();
    sVisibleReadingForTesting = visible;
}
#endif

bool32 IsBattleTextReadingHeadless(void)
{
#if TESTING
    if (sVisibleReadingForTesting)
        return FALSE;
#endif
    return gTestRunnerHeadless;
}

bool32 IsBattleTextReadingEligible(u32 battleTypeFlags, bool32 headless)
{
    return !headless && !(battleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED));
}

void ResetBattleTextReading(void)
{
    ResetTextReading(&sBattleTextReading);
}

void BeginBattleTextReading(void)
{
    if (IsBattleTextReadingEligible(gBattleTypeFlags, IsBattleTextReadingHeadless())
     && !gTextFlags.autoScroll && !AUTO_SCROLL_TEXT)
        BeginTextReading(&sBattleTextReading, B_WIN_MSG);
    else
        ResetBattleTextReading();
}

void UpdateBattleTextReading(void)
{
    UpdateTextReading(&sBattleTextReading,
                      IsBattleTextReadingEligible(gBattleTypeFlags, IsBattleTextReadingHeadless())
                      && !gTextFlags.autoScroll && !AUTO_SCROLL_TEXT);
}

bool32 TryAdvanceBattleTextReading(bool32 presentationBusy)
{
    UpdateBattleTextReading();
    if (!sBattleTextReading.enabled)
        return FALSE;
    return TryAdvanceTextReading(&sBattleTextReading, presentationBusy, A_BUTTON);
}

void FinishBattleTextReading(void)
{
    FinishTextReading(&sBattleTextReading);
}

bool32 IsBattleTextReadingInputUsed(void)
{
    return sBattleTextReading.inputUsed && sBattleTextReading.inputUsedAt == gInputFrameCounter;
}
