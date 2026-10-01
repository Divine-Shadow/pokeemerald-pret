#include "global.h"
#include "main.h"
#include "text.h"
#include "text_reading.h"

#define READING_HOLD_FRAMES 10
#define READING_FINAL_WAIT_FRAMES 6
#define READING_CONTINUATION_FRAMES 4

void ResetTextReading(struct TextReadingState *state)
{
    if (state->enabled)
        ResetTextPrinterBurstState(state->windowId);
    *state = (struct TextReadingState){.windowId = state->windowId};
}

void BeginTextReading(struct TextReadingState *state, u8 windowId)
{
    bool32 continueReading = state->continuation
                          && (u32)(gMain.vblankCounter1 - state->continuationAt) <= READING_CONTINUATION_FRAMES
                          && JOY_HELD(R_BUTTON)
                          && !JOY_NEW(A_BUTTON | B_BUTTON);

    ResetTextReading(state);
    state->windowId = windowId;
    state->enabled = TRUE;
    state->startedAt = gInputFrameCounter;
    state->holdFrames = continueReading ? READING_HOLD_FRAMES : 0;
    // Call and battle strings contain pauses that synchronize their presentation.
    SetTextPrinterPreserveTimedPauses(windowId, TRUE);
}

void UpdateTextReading(struct TextReadingState *state, bool32 eligible)
{
    if (!eligible)
    {
        ResetTextReading(state);
        return;
    }
    // Finish disables the printer owner while a direct continuation is pending.
    // Releases in that gap must cancel qualification before the next Begin.
    if (!JOY_HELD(R_BUTTON))
    {
        state->holdFrames = 0;
        state->finalWaitStarted = FALSE;
        state->continuation = FALSE;
    }
    if (!state->enabled)
        return;
    if (state->updated && state->updatedAt == gInputFrameCounter)
        return;

    state->updated = TRUE;
    state->updatedAt = gInputFrameCounter;
    if (JOY_HELD(R_BUTTON) && state->holdFrames < READING_HOLD_FRAMES)
    {
        state->holdFrames++;
    }

    if (IsTextPrinterActive(state->windowId))
    {
        state->finalWaitStarted = FALSE;
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            // An edge belongs to revealing/advancing this pane, never its final dismissal.
            state->inputUsed = TRUE;
            state->inputUsedAt = gInputFrameCounter;
            if (state->startedAt != gInputFrameCounter && JOY_NEW(A_BUTTON))
                TryCompleteTextPrinterPage(state->windowId);
        }
        SetTextPrinterFastForward(state->windowId, state->holdFrames >= READING_HOLD_FRAMES);
    }
}

bool32 RunTextReadingPrinter(struct TextReadingState *state, bool32 eligible)
{
    u8 windowId = state->windowId;

    UpdateTextReading(state, eligible);
    RunTextPrinters();
    return IsTextPrinterActive(windowId);
}

bool32 TryAdvanceTextReading(struct TextReadingState *state, bool32 presentationBusy, u16 confirmKeys)
{
    if (!state->enabled)
        return !presentationBusy && !IsTextPrinterActive(state->windowId) && JOY_NEW(confirmKeys);
    if (presentationBusy || IsTextPrinterActive(state->windowId))
    {
        state->finalWaitStarted = FALSE;
        return FALSE;
    }
    if (state->startedAt == gInputFrameCounter
     || (state->inputUsed && state->inputUsedAt == gInputFrameCounter))
        return FALSE;
    if (JOY_NEW(confirmKeys))
        return TRUE;
    if (state->holdFrames < READING_HOLD_FRAMES || !JOY_HELD(R_BUTTON))
        return FALSE;

    if (!state->finalWaitStarted)
    {
        state->finalWaitStarted = TRUE;
        state->finalWaitStartedAt = gMain.vblankCounter1;
    }
    return (u32)(gMain.vblankCounter1 - state->finalWaitStartedAt) >= READING_FINAL_WAIT_FRAMES;
}

void FinishTextReading(struct TextReadingState *state)
{
    bool32 continueReading = state->holdFrames >= READING_HOLD_FRAMES
                          && JOY_HELD(R_BUTTON)
                          && !JOY_NEW(A_BUTTON | B_BUTTON);

    ResetTextReading(state);
    state->continuation = continueReading;
    state->continuationAt = gMain.vblankCounter1;
}
