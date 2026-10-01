#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_presentation.h"
#include "main.h"
#include "m4a.h"
#include "match_call.h"
#include "pokenav.h"
#include "sound.h"
#include "text.h"
#include "text_reading.h"
#include "window.h"
#include "test/test.h"
#include "constants/battle.h"
#include "constants/songs.h"

void BS_WaitReadMessage(void);
void BS_WaitMessageOrButton(void);
extern struct MusicPlayerInfo *gMPlay_PokemonCry;

// Native opcode + function pointer + duration + a sentinel next instruction.
static const u8 sReadingWait[] = {0, 0, 0, 0, 0, 3, 0, 0};

static const u8 sCallText[] = _("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
static const u8 sPageText[] = _("ABC\pDEF");
static const u8 sLongText[] = _("ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ");
static const struct WindowTemplate sReadingWindow =
{
    .bg = 0,
    .width = 30,
    .height = 4,
};
static EWRAM_DATA ALIGNED(4) u8 sReadingTiles[30 * 4 * 32];

static void SetUpReading(void)
{
    m4aMPlayStop(&gMPlayInfo_SE1);
    m4aMPlayStop(&gMPlayInfo_SE2);
    if (gMPlay_PokemonCry != NULL)
        StopCryAndClearCrySongs();
    gAnimScriptActive = FALSE;
    DeactivateAllTextPrinters();
    SetDefaultFontsPointer();
    memset(sReadingTiles, 0, sizeof(sReadingTiles));
    gWindows[0].window = gWindows[1].window = sReadingWindow;
    gWindows[0].tileData = gWindows[1].tileData = sReadingTiles;
    gTextFlags = (TextFlags){0};
    gDisableTextPrinters = FALSE;
    gBattleTypeFlags = 0;
    gMain.newKeys = gMain.heldKeys = 0;
    gMain.vblankCounter1 = 0;
    gInputFrameCounter = 0;
}

static void Frame(u16 newKeys, u16 heldKeys)
{
    gMain.vblankCounter1++;
    gInputFrameCounter++;
    gMain.newKeys = newKeys;
    gMain.heldKeys = heldKeys;
}

static void CheckCallCompletion(void (*init)(u8, const u8 *), bool32 (*run)(void), bool32 (*advance)(bool32))
{
    SetUpReading();
    init(1, sCallText);
    Frame(A_BUTTON, A_BUTTON);
    EXPECT(!run());
    EXPECT(!IsTextPrinterActive(1));
    // Interrupts may occur while a burst copies tiles, without sampling new keys.
    gMain.vblankCounter1 += 3;
    EXPECT(!advance(FALSE));
    Frame(0, A_BUTTON);
    run();
    EXPECT(!advance(FALSE));
    Frame(A_BUTTON, A_BUTTON);
    run();
    EXPECT(!advance(TRUE));
    EXPECT(advance(FALSE));
    DeactivateAllTextPrinters();
}

TEST("Call reading/Incoming and scripted call A reveals without hanging up")
{
    CheckCallCompletion(InitIncomingCallPrinterForTesting, RunIncomingCallPrinterForTesting, TryAdvanceIncomingCallForTesting);
}

TEST("Call reading/Pokenav menu call A reveals without hanging up")
{
    CheckCallCompletion(InitPokenavCallPrinterForTesting, RunPokenavCallPrinterForTesting, TryAdvancePokenavCallForTesting);
}

static void CheckCallPage(void (*init)(u8, const u8 *), bool32 (*run)(void), bool32 (*advance)(bool32))
{
    u32 i;

    SetUpReading();
    init(1, sPageText);
    Frame(A_BUTTON, A_BUTTON);
    EXPECT(run());
    EXPECT(!advance(FALSE));
    Frame(0, 0);
    EXPECT(run());
    Frame(A_BUTTON, A_BUTTON);
    EXPECT(run());
    EXPECT(!advance(FALSE));
    for (i = 0; i < 32 && IsTextPrinterActive(1); i++)
    {
        Frame(0, 0);
        run();
    }
    EXPECT(!IsTextPrinterActive(1));
    EXPECT(!advance(FALSE));
    Frame(A_BUTTON, A_BUTTON);
    run();
    EXPECT(advance(FALSE));
    DeactivateAllTextPrinters();
}

TEST("Call reading/Both call paths stop A at page boundaries")
{
    CheckCallPage(InitIncomingCallPrinterForTesting, RunIncomingCallPrinterForTesting, TryAdvanceIncomingCallForTesting);
    CheckCallPage(InitPokenavCallPrinterForTesting, RunPokenavCallPrinterForTesting, TryAdvancePokenavCallForTesting);
}

static void CheckCallR(void (*init)(u8, const u8 *), bool32 (*run)(void), bool32 (*advance)(bool32))
{
    u32 i;
    u32 normalFrames;
    u32 fastFrames;

    SetUpReading();
    init(1, sLongText);
    for (normalFrames = 0; normalFrames < 1000 && IsTextPrinterActive(1); normalFrames++)
    {
        Frame(0, 0);
        run();
    }
    EXPECT(!IsTextPrinterActive(1));
    init(1, sLongText);
    for (fastFrames = 0; fastFrames < 1000 && IsTextPrinterActive(1); fastFrames++)
    {
        Frame(0, R_BUTTON);
        run();
    }
    EXPECT(!IsTextPrinterActive(1));
    EXPECT(fastFrames < normalFrames);
    for (i = 0; i < 12; i++)
    {
        Frame(0, R_BUTTON);
        run();
        EXPECT(!advance(TRUE));
    }
    EXPECT(!advance(FALSE));
    for (i = 0; i < 6; i++)
    {
        Frame(0, R_BUTTON);
        run();
        if (i < 5)
            EXPECT(!advance(FALSE));
    }
    EXPECT(advance(FALSE));
    Frame(0, 0);
    run();
    EXPECT(!advance(FALSE));
    DeactivateAllTextPrinters();
}

TEST("Call reading/Both call paths accelerate with R and dwell after presentation")
{
    CheckCallR(InitIncomingCallPrinterForTesting, RunIncomingCallPrinterForTesting, TryAdvanceIncomingCallForTesting);
    CheckCallR(InitPokenavCallPrinterForTesting, RunPokenavCallPrinterForTesting, TryAdvancePokenavCallForTesting);
}

TEST("Text reading/Opening A and repeated updates cannot reveal or reuse an edge")
{
    struct TextReadingState reading = {0};

    SetUpReading();
    gMain.newKeys = gMain.heldKeys = A_BUTTON;
    AddTextPrinterParameterized(1, FONT_NORMAL, sCallText, 0, 0, 3, NULL);
    BeginTextReading(&reading, 1);
    EXPECT(RunTextReadingPrinter(&reading, TRUE));
    EXPECT(!TryAdvanceTextReading(&reading, FALSE, A_BUTTON));
    Frame(A_BUTTON, A_BUTTON);
    UpdateTextReading(&reading, TRUE);
    UpdateTextReading(&reading, TRUE);
    RunTextPrinters();
    EXPECT(!IsTextPrinterActive(1));
    EXPECT(!TryAdvanceTextReading(&reading, FALSE, A_BUTTON));
    ResetTextReading(&reading);
}

TEST("Text reading/Menu reset and released R discard continuation")
{
    struct TextReadingState reading = {0};
    u32 i;

    SetUpReading();
    AddTextPrinterParameterized(1, FONT_NORMAL, sCallText, 0, 0, 3, NULL);
    BeginTextReading(&reading, 1);
    for (i = 0; i < 20; i++)
    {
        Frame(0, R_BUTTON);
        RunTextReadingPrinter(&reading, TRUE);
    }
    FinishTextReading(&reading);
    ResetTextReading(&reading);
    AddTextPrinterParameterized(1, FONT_NORMAL, sLongText, 0, 0, 3, NULL);
    BeginTextReading(&reading, 1);
    for (i = 0; i < 9; i++)
    {
        Frame(0, R_BUTTON);
        RunTextReadingPrinter(&reading, TRUE);
    }
    EXPECT(IsTextPrinterActive(1));
    Frame(0, 0);
    RunTextReadingPrinter(&reading, TRUE);
    EXPECT(!TryAdvanceTextReading(&reading, FALSE, A_BUTTON));
    ResetTextReading(&reading);
    DeactivateAllTextPrinters();
}

TEST("Text reading/Disabled controls keep the actual dynamic call window")
{
    struct TextReadingState reading = {0};
    u32 i;

    SetUpReading();
    AddTextPrinterParameterized(1, FONT_NORMAL, sCallText, 0, 0, 3, NULL);
    BeginTextReading(&reading, 1);
    for (i = 0; i < 3; i++)
    {
        Frame(A_BUTTON, R_BUTTON | A_BUTTON);
        EXPECT(RunTextReadingPrinter(&reading, FALSE));
        EXPECT(!TryAdvanceTextReading(&reading, FALSE, A_BUTTON));
    }
    DeactivateAllTextPrinters();
}

static void QualifyReadingContinuation(struct TextReadingState *reading)
{
    u32 i;

    AddTextPrinterParameterized(1, FONT_NORMAL, sCallText, 0, 0, 3, NULL);
    BeginTextReading(reading, 1);
    for (i = 0; i < 20; i++)
    {
        Frame(0, R_BUTTON);
        RunTextReadingPrinter(reading, TRUE);
    }
    EXPECT(!IsTextPrinterActive(1));
    FinishTextReading(reading);
}

static bool32 RunNextPaneForSevenFrames(struct TextReadingState *reading)
{
    u32 i;

    AddTextPrinterParameterized(1, FONT_NORMAL, sCallText, 0, 0, 3, NULL);
    BeginTextReading(reading, 1);
    for (i = 0; i < 7; i++)
    {
        Frame(0, R_BUTTON);
        RunTextReadingPrinter(reading, TRUE);
    }
    return IsTextPrinterActive(1);
}

TEST("Text reading/Release and repress in the disabled continuation gap requalifies R")
{
    struct TextReadingState reading = {0};

    SetUpReading();
    QualifyReadingContinuation(&reading);
    Frame(0, 0);
    UpdateTextReading(&reading, TRUE);
    Frame(0, R_BUTTON);
    UpdateTextReading(&reading, TRUE);
    // The old bug completed this pane immediately at the qualified rate.
    EXPECT(RunNextPaneForSevenFrames(&reading));
    ResetTextReading(&reading);
    DeactivateAllTextPrinters();
}

TEST("Text reading/Uninterrupted direct R continuation keeps its qualified rate")
{
    struct TextReadingState reading = {0};

    SetUpReading();
    QualifyReadingContinuation(&reading);
    Frame(0, R_BUTTON);
    UpdateTextReading(&reading, TRUE);
    EXPECT(!RunNextPaneForSevenFrames(&reading));
    ResetTextReading(&reading);
    DeactivateAllTextPrinters();
}

TEST("Text reading/Expired or ineligible disabled continuation requalifies R")
{
    struct TextReadingState reading = {0};
    u32 i;

    SetUpReading();
    QualifyReadingContinuation(&reading);
    for (i = 0; i < 5; i++)
    {
        Frame(0, R_BUTTON);
        UpdateTextReading(&reading, TRUE);
    }
    EXPECT(RunNextPaneForSevenFrames(&reading));
    ResetTextReading(&reading);
    DeactivateAllTextPrinters();
    SetUpReading();
    QualifyReadingContinuation(&reading);
    Frame(0, R_BUTTON);
    UpdateTextReading(&reading, FALSE);
    EXPECT(RunNextPaneForSevenFrames(&reading));
    ResetTextReading(&reading);
    DeactivateAllTextPrinters();
}

TEST("Battle reading/A cannot dismiss its revealed text and animation gates the timeout")
{
    bool32 sameEdgeBlocked;
    bool32 animationBlocked;
    bool32 readyAdvanced;

    // Exercise setup after a preceding scene has left a real SE track active.
    PlaySE(SE_SELECT);
    m4aSoundMain();
    EXPECT(IsSEPlaying());
    SetUpReading();
    SetBattleTextReadingVisibleForTesting(TRUE);
    gBattleControllerExecFlags = 0;
    gBattleCommunication[MSG_DISPLAY] = TRUE;
    gPauseCounterBattle = 0;
    gBattlescriptCurrInstr = sReadingWait;
    AddTextPrinterParameterized(B_WIN_MSG, FONT_NORMAL, sCallText, 0, 0, 3, NULL);
    BeginBattleTextReading();
    Frame(A_BUTTON, A_BUTTON);
    UpdateBattleTextReading();
    RunTextPrinters();
    gMain.vblankCounter1 += 3;
    BS_WaitReadMessage();
    sameEdgeBlocked = gBattlescriptCurrInstr == sReadingWait;
    gAnimScriptActive = TRUE;
    Frame(A_BUTTON, A_BUTTON);
    BS_WaitReadMessage();
    BS_WaitReadMessage();
    animationBlocked = gBattlescriptCurrInstr == sReadingWait;
    gAnimScriptActive = FALSE;
    Frame(0, 0);
    BS_WaitReadMessage();
    readyAdvanced = gBattlescriptCurrInstr == sReadingWait + 7;
    ResetBattleTextReading();
    SetBattleTextReadingVisibleForTesting(FALSE);
    DeactivateAllTextPrinters();
    EXPECT(sameEdgeBlocked);
    EXPECT(animationBlocked);
    EXPECT(readyAdvanced);
}

TEST("Battle reading/Controller and real SE block Intimidate-style confirmation")
{
    bool32 controllerBlocked;
    bool32 soundBlocked;
    bool32 readyAdvanced;

    SetUpReading();
    SetBattleTextReadingVisibleForTesting(TRUE);
    gBattleCommunication[MSG_DISPLAY] = TRUE;
    gPauseCounterBattle = 0;
    gBattlescriptCurrInstr = sReadingWait;
    AddTextPrinterParameterized(B_WIN_MSG, FONT_NORMAL, sCallText, 0, 0, 3, NULL);
    BeginBattleTextReading();
    Frame(A_BUTTON, A_BUTTON);
    UpdateBattleTextReading();
    RunTextPrinters();
    gBattleControllerExecFlags = 1;
    Frame(A_BUTTON, A_BUTTON);
    BS_WaitMessageOrButton();
    controllerBlocked = gBattlescriptCurrInstr == sReadingWait;
    gBattleControllerExecFlags = 0;
    PlaySE(SE_SELECT);
    m4aSoundMain();
    BS_WaitMessageOrButton();
    soundBlocked = IsSEPlaying() && gBattlescriptCurrInstr == sReadingWait;
    m4aMPlayStop(&gMPlayInfo_SE1);
    m4aMPlayStop(&gMPlayInfo_SE2);
    Frame(A_BUTTON, A_BUTTON);
    BS_WaitMessageOrButton();
    readyAdvanced = gBattlescriptCurrInstr == sReadingWait + 7;
    ResetBattleTextReading();
    SetBattleTextReadingVisibleForTesting(FALSE);
    DeactivateAllTextPrinters();
    EXPECT(controllerBlocked);
    EXPECT(soundBlocked);
    EXPECT(readyAdvanced);
}

TEST("Battle reading/Link and recorded native waits retain their original counter timing")
{
    bool32 firstBlocked[2];
    bool32 secondBlocked[2];
    bool32 thirdAdvanced[2];
    u32 types[] = {BATTLE_TYPE_LINK, BATTLE_TYPE_RECORDED};
    u32 i;

    SetUpReading();
    SetBattleTextReadingVisibleForTesting(TRUE);
    gBattleControllerExecFlags = 0;
    // The original waitmessage does not reinterpret animation time as reading.
    gAnimScriptActive = TRUE;
    for (i = 0; i < ARRAY_COUNT(types); i++)
    {
        gBattleTypeFlags = types[i];
        gBattleCommunication[MSG_DISPLAY] = TRUE;
        gPauseCounterBattle = 0;
        gBattlescriptCurrInstr = sReadingWait;
        Frame(A_BUTTON, A_BUTTON | R_BUTTON);
        BS_WaitReadMessage();
        firstBlocked[i] = gPauseCounterBattle == 1 && gBattlescriptCurrInstr == sReadingWait;
        BS_WaitReadMessage();
        secondBlocked[i] = gPauseCounterBattle == 2 && gBattlescriptCurrInstr == sReadingWait;
        BS_WaitReadMessage();
        thirdAdvanced[i] = gPauseCounterBattle == 0 && gBattlescriptCurrInstr == sReadingWait + 7;
    }
    gAnimScriptActive = FALSE;
    gBattleTypeFlags = 0;
    SetBattleTextReadingVisibleForTesting(FALSE);
    for (i = 0; i < ARRAY_COUNT(types); i++)
    {
        EXPECT(firstBlocked[i]);
        EXPECT(secondBlocked[i]);
        EXPECT(thirdAdvanced[i]);
    }
}

TEST("Battle reading/Active cry channels block fresh A until they finish")
{
    bool32 cryBlocked;
    bool32 readyAdvanced;
    struct MusicPlayerTrack track = {0};
    struct SoundChannel channel = {.track = &track};
    struct MusicPlayerInfo cry = {.tracks = &track};
    struct MusicPlayerInfo *oldCry = gMPlay_PokemonCry;

    SetUpReading();
    SetBattleTextReadingVisibleForTesting(TRUE);
    gBattleControllerExecFlags = 0;
    gBattleCommunication[MSG_DISPLAY] = TRUE;
    gPauseCounterBattle = 0;
    gBattlescriptCurrInstr = sReadingWait;
    AddTextPrinterParameterized(B_WIN_MSG, FONT_NORMAL, sCallText, 0, 0, 3, NULL);
    BeginBattleTextReading();
    Frame(A_BUTTON, A_BUTTON);
    UpdateBattleTextReading();
    RunTextPrinters();
    // Exercise the real cry predicate with a deterministically active channel.
    // Isolated headless playback does not allocate a cry channel.
    track.chan = &channel;
    gMPlay_PokemonCry = &cry;
    Frame(A_BUTTON, A_BUTTON);
    BS_WaitReadMessage();
    cryBlocked = IsCryPlaying() && gBattlescriptCurrInstr == sReadingWait;
    track.chan = NULL;
    Frame(A_BUTTON, A_BUTTON);
    BS_WaitReadMessage();
    readyAdvanced = gBattlescriptCurrInstr == sReadingWait + 7;
    gMPlay_PokemonCry = NULL;
    readyAdvanced = readyAdvanced && !IsCryPlaying();
    gMPlay_PokemonCry = oldCry;
    ResetBattleTextReading();
    SetBattleTextReadingVisibleForTesting(FALSE);
    DeactivateAllTextPrinters();
    EXPECT(cryBlocked);
    EXPECT(readyAdvanced);
}
