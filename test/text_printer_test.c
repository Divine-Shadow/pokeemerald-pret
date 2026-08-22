#include "global.h"
#include "battle.h"
#include "main.h"
#include "text.h"
#include "window.h"
#include "test/test.h"

static const u8 sPageBoundaryText[] = _("ABC\pDEF");
static const u8 sGlyphCountingText[] = _("A{CLEAR 1}BCDE");
static const u8 sDwellText[] = _("A\pB");
static const u8 sPauseText[] = _("A{PAUSE 30}B");
static const u8 sCustomFontText[] = _("A");
static const u8 sReplacementText[] = _("Z");

static const struct WindowTemplate sTextPrinterTestWindow =
{
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 0,
    .width = 30,
    .height = 4,
    .paletteNum = 0,
    .baseBlock = 0,
};

static EWRAM_DATA ALIGNED(4) u8 sTextPrinterTestTiles[30 * 4 * 32];
static u32 sPrintCallbacks;
static u32 sUpdateCallbacks;
static u32 sCustomFontCalls;
static const u8 *sLastPrinterChar;

static void RecordTextPrinterCallback(struct TextPrinterTemplate *printer, u16 renderCmd)
{
    sLastPrinterChar = printer->currentChar;
    if (renderCmd == RENDER_PRINT)
        sPrintCallbacks++;
    else if (renderCmd == RENDER_UPDATE)
        sUpdateCallbacks++;
}

static void ChangeTextPrinterWindowCallback(struct TextPrinterTemplate *printer, u16 renderCmd)
{
    RecordTextPrinterCallback(printer, renderCmd);
    printer->windowId = 1;
}

static void ReplaceTextPrinterCallback(struct TextPrinterTemplate *printer, u16 renderCmd)
{
    RecordTextPrinterCallback(printer, renderCmd);
    AddTextPrinterParameterized(printer->windowId, FONT_NORMAL, sReplacementText, 0, 0, 3, NULL);
}

static u16 FontFunc_NoProgress(struct TextPrinter * UNUSED textPrinter)
{
    sCustomFontCalls++;
    return RENDER_UPDATE;
}

static u16 FontFunc_MalformedRepeat(struct TextPrinter * UNUSED textPrinter)
{
    sCustomFontCalls++;
    return RENDER_REPEAT;
}

static const struct FontInfo sNoProgressFont[] =
{
    {
        .fontFunction = FontFunc_NoProgress,
        .maxLetterWidth = 8,
        .maxLetterHeight = 16,
        .fgColor = TEXT_COLOR_DARK_GRAY,
        .bgColor = TEXT_COLOR_TRANSPARENT,
        .shadowColor = TEXT_COLOR_LIGHT_GRAY,
    },
};

static const struct FontInfo sMalformedRepeatFont[] =
{
    {
        .fontFunction = FontFunc_MalformedRepeat,
        .maxLetterWidth = 8,
        .maxLetterHeight = 16,
        .fgColor = TEXT_COLOR_DARK_GRAY,
        .bgColor = TEXT_COLOR_TRANSPARENT,
        .shadowColor = TEXT_COLOR_LIGHT_GRAY,
    },
};

static void SetUpTextPrinterTest(void)
{
    DeactivateAllTextPrinters();
    SetDefaultFontsPointer();
    memset(sTextPrinterTestTiles, 0, sizeof(sTextPrinterTestTiles));
    gWindows[0].window = sTextPrinterTestWindow;
    gWindows[0].tileData = sTextPrinterTestTiles;
    gTextFlags = (TextFlags){0};
    gDisableTextPrinters = FALSE;
    gBattleTypeFlags = 0;
    gMain.newKeys = 0;
    gMain.heldKeys = 0;
    sPrintCallbacks = 0;
    sUpdateCallbacks = 0;
    sCustomFontCalls = 0;
    sLastPrinterChar = NULL;
}

static const u8 *FindTextControl(const u8 *text, u8 control)
{
    while (*text != EOS && *text != control)
        text++;
    return text;
}

TEST("Dialogue text printer/A completion stops at a page boundary")
{
    const u8 *boundary;
    u32 i;

    SetUpTextPrinterTest();
    AddTextPrinterParameterized(0, FONT_NORMAL, sPageBoundaryText, 0, 0, 3, RecordTextPrinterCallback);
    boundary = FindTextControl(sPageBoundaryText, CHAR_PROMPT_CLEAR);

    EXPECT(TryCompleteTextPrinterPage(0));
    RunTextPrinters();

    EXPECT(IsTextPrinterActive(0));
    EXPECT_EQ(sPrintCallbacks, 3);
    EXPECT_EQ(sLastPrinterChar, boundary + 1);
    EXPECT(!TryCompleteTextPrinterPage(0));

    RunTextPrinters();
    EXPECT(IsTextPrinterActive(0));
    EXPECT_EQ(sPrintCallbacks, 3);

    gMain.newKeys = A_BUTTON;
    gMain.heldKeys = A_BUTTON;
    RunTextPrinters();
    gMain.newKeys = 0;
    gMain.heldKeys = 0;
    for (i = 0; i < 3; i++)
        RunTextPrinters();

    EXPECT_EQ(sPrintCallbacks, 4);
    DeactivateAllTextPrinters();
}

TEST("Dialogue text printer/Fast forward counts glyphs instead of print controls")
{
    SetUpTextPrinterTest();
    AddTextPrinterParameterized(0, FONT_NORMAL, sGlyphCountingText, 0, 0, 3, RecordTextPrinterCallback);
    SetTextPrinterFastForward(0, TRUE);

    RunTextPrinters();

    EXPECT(IsTextPrinterActive(0));
    EXPECT_EQ(sPrintCallbacks, 5);

    RunTextPrinters();
    EXPECT(!IsTextPrinterActive(0));
    EXPECT_EQ(sPrintCallbacks, 6);
}

TEST("Dialogue text printer/Fast forward dwells six frames at a page boundary")
{
    u32 i;

    SetUpTextPrinterTest();
    AddTextPrinterParameterized(0, FONT_NORMAL, sDwellText, 0, 0, 3, RecordTextPrinterCallback);
    SetTextPrinterFastForward(0, TRUE);
    RunTextPrinters();
    EXPECT_EQ(sPrintCallbacks, 1);

    for (i = 0; i < 5; i++)
        RunTextPrinters();
    EXPECT(IsTextPrinterActive(0));
    EXPECT_EQ(sPrintCallbacks, 1);

    RunTextPrinters();
    EXPECT(IsTextPrinterActive(0));
    EXPECT_EQ(sPrintCallbacks, 1);

    RunTextPrinters();
    EXPECT(!IsTextPrinterActive(0));
    EXPECT_EQ(sPrintCallbacks, 2);
}

TEST("Dialogue text printer/A completion collapses timed pauses and preserves callbacks")
{
    SetUpTextPrinterTest();
    AddTextPrinterParameterized(0, FONT_NORMAL, sPauseText, 0, 0, 3, RecordTextPrinterCallback);

    EXPECT(TryCompleteTextPrinterPage(0));
    RunTextPrinters();

    EXPECT(!IsTextPrinterActive(0));
    EXPECT_EQ(sPrintCallbacks, 2);
    EXPECT_EQ(sUpdateCallbacks, 1);
}

TEST("Dialogue text printer/Callback window changes cancel only burst rendering")
{
    SetUpTextPrinterTest();
    AddTextPrinterParameterized(0, FONT_NORMAL, sCustomFontText, 0, 0, 3, ChangeTextPrinterWindowCallback);
    SetTextPrinterFastForward(0, TRUE);

    RunTextPrinters();

    EXPECT(IsTextPrinterActive(0));
    EXPECT_EQ(sPrintCallbacks, 1);
    DeactivateAllTextPrinters();
}

TEST("Dialogue text printer/Callback replacement on the same window ends the old burst")
{
    SetUpTextPrinterTest();
    AddTextPrinterParameterized(0, FONT_NORMAL, sCustomFontText, 0, 0, 3, ReplaceTextPrinterCallback);
    SetTextPrinterFastForward(0, TRUE);

    RunTextPrinters();

    EXPECT(IsTextPrinterActive(0));
    EXPECT_EQ(sPrintCallbacks, 1);
    RunTextPrinters();
    EXPECT(IsTextPrinterActive(0));
    DeactivateAllTextPrinters();
}

TEST("Dialogue text printer/A no-progress update cancels burst without finishing")
{
    SetUpTextPrinterTest();
    gFonts = sNoProgressFont;
    AddTextPrinterParameterized(0, 0, sCustomFontText, 0, 0, 3, RecordTextPrinterCallback);
    SetTextPrinterFastForward(0, TRUE);

    RunTextPrinters();

    EXPECT(IsTextPrinterActive(0));
    EXPECT_EQ(sCustomFontCalls, 1);
    EXPECT_EQ(sUpdateCallbacks, 1);
    DeactivateAllTextPrinters();
    SetDefaultFontsPointer();
}

TEST("Dialogue text printer/Malformed repeat renderer hits the safety cap")
{
    SetUpTextPrinterTest();
    gFonts = sMalformedRepeatFont;
    AddTextPrinterParameterized(0, 0, sCustomFontText, 0, 0, 3, NULL);
    SetTextPrinterFastForward(0, TRUE);

    RunTextPrinters();

    EXPECT(IsTextPrinterActive(0));
    EXPECT_EQ(sCustomFontCalls, 0x400);
    RunTextPrinters();
    EXPECT(IsTextPrinterActive(0));
    EXPECT_EQ(sCustomFontCalls, 0x800);
    DeactivateAllTextPrinters();
    SetDefaultFontsPointer();
}
