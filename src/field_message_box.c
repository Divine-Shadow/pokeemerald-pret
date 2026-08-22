#include "global.h"
#include "main.h"
#include "menu.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "match_call.h"
#include "field_message_box.h"
#include "overworld.h"
#include "text_window.h"
#include "script.h"

static EWRAM_DATA u8 sFieldMessageBoxMode = 0;
EWRAM_DATA u8 gWalkAwayFromSignpostTimer = 0;

#define FIELD_MESSAGE_FAST_FORWARD_ACTIVATION_FRAMES 10
#define FIELD_MESSAGE_FAST_FORWARD_FINAL_WAIT_FRAMES 6
#define FIELD_MESSAGE_FAST_FORWARD_FINAL_WAIT_WINDOW 12
#define FIELD_MESSAGE_FAST_FORWARD_CONTINUATION_WINDOW 4

enum
{
    FIELD_MESSAGE_FAST_FORWARD_INACTIVE,
    FIELD_MESSAGE_FAST_FORWARD_ARMING,
    FIELD_MESSAGE_FAST_FORWARD_ACTIVE,
    FIELD_MESSAGE_FAST_FORWARD_PENDING_FINAL_WAIT,
    FIELD_MESSAGE_FAST_FORWARD_FINAL_WAIT,
    FIELD_MESSAGE_FAST_FORWARD_CONTINUATION,
};

struct FieldMessageFastForward
{
    u8 state;
    u8 holdFrames;
    u32 deadline;
    u32 finalWaitReadyAt;
};

static EWRAM_DATA struct FieldMessageFastForward sFieldMessageFastForward = {0};
static EWRAM_DATA bool8 sFieldMessageAccelerationEligible = FALSE;

static void ExpandStringAndStartDrawFieldMessage(const u8 *, bool32);
static void StartDrawFieldMessage(void);
static void ResetFieldMessageAcceleration(void);
static void PrepareFastForwardForFieldMessage(bool8);
static bool8 UpdateFieldMessageFastForward(void);
static void BeginFastForwardFinalWait(u8);
static void Task_WatchFieldMessageFastForward(u8);

static bool8 IsFastForwardDeadlineActive(void)
{
    return (s32)(sFieldMessageFastForward.deadline - gMain.vblankCounter1) > 0;
}

static bool8 IsFastForwardFinalWaitReady(void)
{
    return (s32)(gMain.vblankCounter1 - sFieldMessageFastForward.finalWaitReadyAt) >= 0;
}

void CancelFieldMessageFastForward(void)
{
    u8 taskId = FindTaskIdByFunc(Task_WatchFieldMessageFastForward);

    if (taskId != TASK_NONE)
        DestroyTask(taskId);
    sFieldMessageFastForward.state = FIELD_MESSAGE_FAST_FORWARD_INACTIVE;
    sFieldMessageFastForward.holdFrames = 0;
    sFieldMessageFastForward.deadline = 0;
    sFieldMessageFastForward.finalWaitReadyAt = 0;
    SetTextPrinterFastForward(0, FALSE);
}

static void ResetFieldMessageAcceleration(void)
{
    CancelFieldMessageFastForward();
    ResetTextPrinterBurstState(0);
    sFieldMessageAccelerationEligible = FALSE;
}

static void PrepareFastForwardForFieldMessage(bool8 eligible)
{
    if (!eligible || IsOverworldLinkActive() || AUTO_SCROLL_TEXT || gTextFlags.autoScroll)
    {
        ResetFieldMessageAcceleration();
        return;
    }

    bool8 continueFastForward = (sFieldMessageFastForward.state == FIELD_MESSAGE_FAST_FORWARD_CONTINUATION
                              && IsFastForwardDeadlineActive()
                              && JOY_HELD(R_BUTTON));

    ResetFieldMessageAcceleration();
    sFieldMessageAccelerationEligible = TRUE;
    if (continueFastForward)
    {
        sFieldMessageFastForward.state = FIELD_MESSAGE_FAST_FORWARD_ACTIVE;
        sFieldMessageFastForward.holdFrames = FIELD_MESSAGE_FAST_FORWARD_ACTIVATION_FRAMES;
    }
}

static bool8 UpdateFieldMessageFastForward(void)
{
    switch (sFieldMessageFastForward.state)
    {
    case FIELD_MESSAGE_FAST_FORWARD_INACTIVE:
        if (JOY_NEW(R_BUTTON))
        {
            sFieldMessageFastForward.state = FIELD_MESSAGE_FAST_FORWARD_ARMING;
            sFieldMessageFastForward.holdFrames = 1;
        }
        break;
    case FIELD_MESSAGE_FAST_FORWARD_ARMING:
        if (!JOY_HELD(R_BUTTON))
        {
            CancelFieldMessageFastForward();
        }
        else
        {
            if (sFieldMessageFastForward.holdFrames < FIELD_MESSAGE_FAST_FORWARD_ACTIVATION_FRAMES)
                sFieldMessageFastForward.holdFrames++;
            if (sFieldMessageFastForward.holdFrames >= FIELD_MESSAGE_FAST_FORWARD_ACTIVATION_FRAMES)
                sFieldMessageFastForward.state = FIELD_MESSAGE_FAST_FORWARD_ACTIVE;
        }
        break;
    case FIELD_MESSAGE_FAST_FORWARD_ACTIVE:
        if (!JOY_HELD(R_BUTTON))
            CancelFieldMessageFastForward();
        break;
    default:
        CancelFieldMessageFastForward();
        break;
    }

    return sFieldMessageFastForward.state == FIELD_MESSAGE_FAST_FORWARD_ACTIVE;
}

static void BeginFastForwardFinalWait(u8 taskId)
{
    sFieldMessageFastForward.state = FIELD_MESSAGE_FAST_FORWARD_PENDING_FINAL_WAIT;
    sFieldMessageFastForward.deadline = gMain.vblankCounter1 + FIELD_MESSAGE_FAST_FORWARD_FINAL_WAIT_WINDOW;
    sFieldMessageFastForward.finalWaitReadyAt = gMain.vblankCounter1 + FIELD_MESSAGE_FAST_FORWARD_FINAL_WAIT_FRAMES;
    gTasks[taskId].func = Task_WatchFieldMessageFastForward;
}

static void Task_WatchFieldMessageFastForward(u8 UNUSED taskId)
{
    if (!JOY_HELD(R_BUTTON)
     || ((sFieldMessageFastForward.state == FIELD_MESSAGE_FAST_FORWARD_PENDING_FINAL_WAIT
       || sFieldMessageFastForward.state == FIELD_MESSAGE_FAST_FORWARD_CONTINUATION)
      && !IsFastForwardDeadlineActive()))
    {
        CancelFieldMessageFastForward();
    }
}

bool8 TryFastForwardFieldMessageFinalWait(void)
{
    if (sFieldMessageFastForward.state == FIELD_MESSAGE_FAST_FORWARD_CONTINUATION)
    {
        if (!IsFastForwardDeadlineActive() || !JOY_HELD(R_BUTTON))
            CancelFieldMessageFastForward();
        return FALSE;
    }

    if (sFieldMessageFastForward.state == FIELD_MESSAGE_FAST_FORWARD_PENDING_FINAL_WAIT)
    {
        if (!IsFastForwardDeadlineActive() || !JOY_HELD(R_BUTTON))
        {
            CancelFieldMessageFastForward();
            return FALSE;
        }

        sFieldMessageFastForward.state = FIELD_MESSAGE_FAST_FORWARD_FINAL_WAIT;
    }

    if (sFieldMessageFastForward.state != FIELD_MESSAGE_FAST_FORWARD_FINAL_WAIT)
        return FALSE;

    if (!JOY_HELD(R_BUTTON))
    {
        CancelFieldMessageFastForward();
        return FALSE;
    }

    if (!IsFastForwardFinalWaitReady())
        return FALSE;

    sFieldMessageFastForward.state = FIELD_MESSAGE_FAST_FORWARD_CONTINUATION;
    sFieldMessageFastForward.deadline = gMain.vblankCounter1 + FIELD_MESSAGE_FAST_FORWARD_CONTINUATION_WINDOW;
    return TRUE;
}

void InitFieldMessageBox(void)
{
    sFieldMessageBoxMode = FIELD_MESSAGE_BOX_HIDDEN;
    ResetFieldMessageAcceleration();
    gTextFlags.canABSpeedUpPrint = FALSE;
    gTextFlags.useAlternateDownArrow = FALSE;
    gTextFlags.autoScroll = FALSE;
    gTextFlags.forceMidTextSpeed = FALSE;
}

#define tState data[0]

static void Task_DrawFieldMessage(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (sFieldMessageFastForward.state == FIELD_MESSAGE_FAST_FORWARD_ACTIVE && !JOY_HELD(R_BUTTON))
        CancelFieldMessageFastForward();

    switch (task->tState)
    {
        case 0:
            if (gMsgIsSignPost)
                LoadSignPostWindowFrameGfx();
            else
                LoadMessageBoxAndBorderGfx();
            task->tState++;
            break;
        case 1:
           DrawDialogueFrame(0, TRUE);
           task->tState++;
           break;
        case 2:
        {
            bool8 fastForwardActive = FALSE;

            if (sFieldMessageAccelerationEligible
             && (IsOverworldLinkActive() || AUTO_SCROLL_TEXT || gTextFlags.autoScroll))
                ResetFieldMessageAcceleration();

            if (sFieldMessageBoxMode == FIELD_MESSAGE_BOX_NORMAL
             && sFieldMessageAccelerationEligible
             && !AUTO_SCROLL_TEXT
             && !gTextFlags.autoScroll)
            {
                if (JOY_NEW(A_BUTTON))
                    TryCompleteTextPrinterPage(0);
                fastForwardActive = UpdateFieldMessageFastForward();
                SetTextPrinterFastForward(0, fastForwardActive);
            }

            if (RunTextPrintersAndIsPrinter0Active() != TRUE)
            {
                if (fastForwardActive && JOY_HELD(R_BUTTON))
                    BeginFastForwardFinalWait(taskId);
                else
                {
                    CancelFieldMessageFastForward();
                    DestroyTask(taskId);
                }
                sFieldMessageBoxMode = FIELD_MESSAGE_BOX_HIDDEN;
            }
            break;
        }
    }
}

#undef tState

static void CreateTask_DrawFieldMessage(void)
{
    CreateTask(Task_DrawFieldMessage, 0x50);
}

static void DestroyTask_DrawFieldMessage(void)
{
    u8 taskId = FindTaskIdByFunc(Task_DrawFieldMessage);
    if (taskId != TASK_NONE)
        DestroyTask(taskId);
}

bool8 ShowFieldMessage(const u8 *str)
{
    if (sFieldMessageBoxMode != FIELD_MESSAGE_BOX_HIDDEN)
        return FALSE;
    PrepareFastForwardForFieldMessage(TRUE);
    ExpandStringAndStartDrawFieldMessage(str, TRUE);
    sFieldMessageBoxMode = FIELD_MESSAGE_BOX_NORMAL;
    return TRUE;
}

bool8 ShowFieldMessageNoFastForward(const u8 *str)
{
    if (sFieldMessageBoxMode != FIELD_MESSAGE_BOX_HIDDEN)
        return FALSE;
    PrepareFastForwardForFieldMessage(FALSE);
    ExpandStringAndStartDrawFieldMessage(str, TRUE);
    sFieldMessageBoxMode = FIELD_MESSAGE_BOX_NORMAL;
    return TRUE;
}

static void Task_HidePokenavMessageWhenDone(u8 taskId)
{
    if (!IsMatchCallTaskActive())
    {
        sFieldMessageBoxMode = FIELD_MESSAGE_BOX_HIDDEN;
        DestroyTask(taskId);
    }
}

bool8 ShowPokenavFieldMessage(const u8 *str)
{
    if (sFieldMessageBoxMode != FIELD_MESSAGE_BOX_HIDDEN)
        return FALSE;
    ResetFieldMessageAcceleration();
    StringExpandPlaceholders(gStringVar4, str);
    CreateTask(Task_HidePokenavMessageWhenDone, 0);
    StartMatchCallFromScript(str);
    sFieldMessageBoxMode = FIELD_MESSAGE_BOX_NORMAL;
    return TRUE;
}

bool8 ShowFieldAutoScrollMessage(const u8 *str)
{
    if (sFieldMessageBoxMode != FIELD_MESSAGE_BOX_HIDDEN)
        return FALSE;
    ResetFieldMessageAcceleration();
    sFieldMessageBoxMode = FIELD_MESSAGE_BOX_AUTO_SCROLL;
    ExpandStringAndStartDrawFieldMessage(str, FALSE);
    return TRUE;
}

static bool8 UNUSED ForceShowFieldAutoScrollMessage(const u8 *str)
{
    ResetFieldMessageAcceleration();
    sFieldMessageBoxMode = FIELD_MESSAGE_BOX_AUTO_SCROLL;
    ExpandStringAndStartDrawFieldMessage(str, TRUE);
    return TRUE;
}

// Same as ShowFieldMessage, but instead of accepting a
// string arg it just prints whats already in gStringVar4
bool8 ShowFieldMessageFromBuffer(void)
{
    if (sFieldMessageBoxMode != FIELD_MESSAGE_BOX_HIDDEN)
        return FALSE;
    PrepareFastForwardForFieldMessage(TRUE);
    sFieldMessageBoxMode = FIELD_MESSAGE_BOX_NORMAL;
    StartDrawFieldMessage();
    return TRUE;
}

static void ExpandStringAndStartDrawFieldMessage(const u8 *str, bool32 allowSkippingDelayWithButtonPress)
{
    StringExpandPlaceholders(gStringVar4, str);
    AddTextPrinterForMessage(allowSkippingDelayWithButtonPress);
    CreateTask_DrawFieldMessage();
}

static void StartDrawFieldMessage(void)
{
    AddTextPrinterForMessage(TRUE);
    CreateTask_DrawFieldMessage();
}

void HideFieldMessageBox(void)
{
    ResetFieldMessageAcceleration();
    DestroyTask_DrawFieldMessage();
    ClearDialogWindowAndFrame(0, TRUE);
    sFieldMessageBoxMode = FIELD_MESSAGE_BOX_HIDDEN;
}

u8 GetFieldMessageBoxMode(void)
{
    return sFieldMessageBoxMode;
}

bool8 IsFieldMessageBoxHidden(void)
{
    if (sFieldMessageBoxMode == FIELD_MESSAGE_BOX_HIDDEN)
        return TRUE;
    return FALSE;
}

static void UNUSED ReplaceFieldMessageWithFrame(void)
{
    ResetFieldMessageAcceleration();
    DestroyTask_DrawFieldMessage();
    DrawStdWindowFrame(0, TRUE);
    sFieldMessageBoxMode = FIELD_MESSAGE_BOX_HIDDEN;
}

void StopFieldMessage(void)
{
    ResetFieldMessageAcceleration();
    DestroyTask_DrawFieldMessage();
    sFieldMessageBoxMode = FIELD_MESSAGE_BOX_HIDDEN;
}
