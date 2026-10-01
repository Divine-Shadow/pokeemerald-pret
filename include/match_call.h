#ifndef GUARD_MATCH_CALL_H
#define GUARD_MATCH_CALL_H

enum {
    MATCH_CALL_TYPE_NON_TRAINER,
    MATCH_CALL_TYPE_TRAINER,
    MATCH_CALL_TYPE_WALLY,
    MATCH_CALL_TYPE_BIRCH,
    MATCH_CALL_TYPE_MAY_BRENDAN,
    MATCH_CALL_TYPE_GYMLEADER_ELITEFOUR
};

s32 GetRematchIdxByTrainerIdx(s32 trainerIdx);
void InitMatchCallCounters(void);
bool32 TryStartMatchCall(void);
bool32 IsMatchCallTaskActive(void);
void StartMatchCallFromScript(const u8 *message);
void BufferPokedexRatingForMatchCall(u8 *destStr);
bool32 SelectMatchCallMessage(int trainerId, u8 *str);
void LoadMatchCallWindowGfx(u32 windowId, u32 destOffset, u32 paletteId);
void DrawMatchCallTextBoxBorder(u32 windowId, u32 tileOffset, u32 paletteId);

#if TESTING
void InitIncomingCallPrinterForTesting(u8 windowId, const u8 *str);
bool32 RunIncomingCallPrinterForTesting(void);
bool32 TryAdvanceIncomingCallForTesting(bool32 presentationBusy);
#endif

#endif //GUARD_MATCH_CALL_H
