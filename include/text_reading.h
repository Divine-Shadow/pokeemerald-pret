#ifndef GUARD_TEXT_READING_H
#define GUARD_TEXT_READING_H

struct TextReadingState
{
    u32 startedAt;
    u32 updatedAt;
    u32 inputUsedAt;
    u32 finalWaitStartedAt;
    u32 continuationAt;
    u8 windowId;
    u8 holdFrames;
    bool8 enabled;
    bool8 updated;
    bool8 inputUsed;
    bool8 finalWaitStarted;
    bool8 continuation;
};

void ResetTextReading(struct TextReadingState *state);
void BeginTextReading(struct TextReadingState *state, u8 windowId);
void UpdateTextReading(struct TextReadingState *state, bool32 eligible);
bool32 RunTextReadingPrinter(struct TextReadingState *state, bool32 eligible);
bool32 TryAdvanceTextReading(struct TextReadingState *state, bool32 presentationBusy, u16 confirmKeys);
void FinishTextReading(struct TextReadingState *state);

#endif // GUARD_TEXT_READING_H
