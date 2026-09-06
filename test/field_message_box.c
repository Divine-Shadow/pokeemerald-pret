#include "global.h"
#include "field_message_box.h"
#include "main.h"
#include "test/test.h"

#define FAST_FORWARD_ACTIVATION_FRAMES 10
#define FAST_FORWARD_FINAL_WAIT_FRAMES 6
#define FAST_FORWARD_FINAL_WAIT_WINDOW 12

static void SetUpFastForwardTest(void)
{
    gMain.newKeys = 0;
    gMain.heldKeys = 0;
    gMain.vblankCounter1 = 0;
    PrepareFieldMessageFastForwardForTesting();
}

static void TearDownFastForwardTest(void)
{
    gMain.newKeys = 0;
    gMain.heldKeys = 0;
    gMain.vblankCounter1 = 0;
    CancelFieldMessageFastForward();
}

static void HoldR(void)
{
    gMain.newKeys = 0;
    gMain.heldKeys = R_BUTTON;
}

static void ExpectRHoldActivatesAfterTenFrames(void)
{
    u32 i;

    for (i = 0; i < FAST_FORWARD_ACTIVATION_FRAMES - 1; i++)
        EXPECT(!UpdateFieldMessageFastForwardForTesting());
    EXPECT(UpdateFieldMessageFastForwardForTesting());
}

static void QualifyAndBeginFinalWait(void)
{
    HoldR();
    ExpectRHoldActivatesAfterTenFrames();
    BeginFieldMessageFastForwardFinalWaitForTesting();
}

static void RunWatcherFrames(u32 frames)
{
    u32 i;

    for (i = 0; i < frames; i++)
    {
        gMain.vblankCounter1++;
        TickFieldMessageFastForwardWatcherForTesting();
    }
}

TEST("Field message fast-forward/Pre-held R qualifies after ten eligible frames")
{
    SetUpFastForwardTest();
    HoldR();

    ExpectRHoldActivatesAfterTenFrames();

    TearDownFastForwardTest();
}

TEST("Field message fast-forward/Releasing R while arming resets qualification")
{
    u32 i;

    SetUpFastForwardTest();
    HoldR();
    for (i = 0; i < 5; i++)
        EXPECT(!UpdateFieldMessageFastForwardForTesting());

    gMain.heldKeys = 0;
    EXPECT(!UpdateFieldMessageFastForwardForTesting());

    HoldR();
    ExpectRHoldActivatesAfterTenFrames();

    TearDownFastForwardTest();
}

TEST("Field message fast-forward/Releasing R while active cancels acceleration")
{
    SetUpFastForwardTest();
    HoldR();
    ExpectRHoldActivatesAfterTenFrames();

    gMain.heldKeys = 0;
    EXPECT(!UpdateFieldMessageFastForwardForTesting());

    HoldR();
    EXPECT(!UpdateFieldMessageFastForwardForTesting());

    TearDownFastForwardTest();
}

TEST("Field message fast-forward/Held R reacquires after a script cancellation")
{
    SetUpFastForwardTest();
    HoldR();
    ExpectRHoldActivatesAfterTenFrames();

    CancelFieldMessageFastForward();
    ExpectRHoldActivatesAfterTenFrames();

    TearDownFastForwardTest();
}

TEST("Field message fast-forward/Presentation begun before qualification preserves the final wait")
{
    SetUpFastForwardTest();
    HoldR();
    BeginFieldMessageFastForwardPresentationWait();
    ExpectRHoldActivatesAfterTenFrames();
    BeginFieldMessageFastForwardFinalWaitForTesting();

    RunWatcherFrames(FAST_FORWARD_FINAL_WAIT_WINDOW + 1);
    EXPECT(!TryFastForwardFieldMessageFinalWait());
    EndFieldMessageFastForwardPresentationWait();
    EXPECT(TryFastForwardFieldMessageFinalWait());
    EXPECT_EQ(gMain.newKeys, 0);

    TearDownFastForwardTest();
}

TEST("Field message fast-forward/Presentation begun after EOS preserves the final wait")
{
    SetUpFastForwardTest();
    QualifyAndBeginFinalWait();
    BeginFieldMessageFastForwardPresentationWait();

    RunWatcherFrames(FAST_FORWARD_FINAL_WAIT_WINDOW + 1);
    EXPECT(!TryFastForwardFieldMessageFinalWait());
    EndFieldMessageFastForwardPresentationWait();
    EXPECT(TryFastForwardFieldMessageFinalWait());

    TearDownFastForwardTest();
}

TEST("Field message fast-forward/Releasing R during presentation discards the final wait")
{
    SetUpFastForwardTest();
    QualifyAndBeginFinalWait();
    BeginFieldMessageFastForwardPresentationWait();

    gMain.heldKeys = 0;
    EXPECT(!TryFastForwardFieldMessageFinalWait());
    EndFieldMessageFastForwardPresentationWait();
    HoldR();
    EXPECT(!TryFastForwardFieldMessageFinalWait());

    TearDownFastForwardTest();
}

TEST("Field message fast-forward/Hard cancellation during presentation cannot resurrect the final wait")
{
    SetUpFastForwardTest();
    QualifyAndBeginFinalWait();
    BeginFieldMessageFastForwardPresentationWait();

    CancelFieldMessageFastForward();
    EndFieldMessageFastForwardPresentationWait();
    HoldR();
    EXPECT(!TryFastForwardFieldMessageFinalWait());

    TearDownFastForwardTest();
}

TEST("Field message fast-forward/Unsuspended final wait expires at frame twelve")
{
    SetUpFastForwardTest();
    QualifyAndBeginFinalWait();

    RunWatcherFrames(FAST_FORWARD_FINAL_WAIT_WINDOW);
    EXPECT(!TryFastForwardFieldMessageFinalWait());

    TearDownFastForwardTest();
}

TEST("Field message fast-forward/Short presentation preserves the final dwell remainder")
{
    SetUpFastForwardTest();
    QualifyAndBeginFinalWait();
    BeginFieldMessageFastForwardPresentationWait();

    RunWatcherFrames(FAST_FORWARD_FINAL_WAIT_FRAMES - 1);
    EndFieldMessageFastForwardPresentationWait();
    EXPECT(!TryFastForwardFieldMessageFinalWait());
    gMain.vblankCounter1++;
    EXPECT(TryFastForwardFieldMessageFinalWait());

    TearDownFastForwardTest();
}

TEST("Field message fast-forward/Automatic final wait does not synthesize button input")
{
    SetUpFastForwardTest();
    QualifyAndBeginFinalWait();

    gMain.vblankCounter1 = FAST_FORWARD_FINAL_WAIT_FRAMES;
    EXPECT(TryFastForwardFieldMessageFinalWait());
    EXPECT_EQ(gMain.newKeys, 0);

    TearDownFastForwardTest();
}
