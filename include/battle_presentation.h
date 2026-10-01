#ifndef GUARD_BATTLE_PRESENTATION_H
#define GUARD_BATTLE_PRESENTATION_H

bool32 ShouldAdvanceBattlePresentationWait(u32 battleTypeFlags, u16 newKeys,
                                           bool32 controllerBusy, bool32 textPrinterActive,
                                           bool32 soundActive, bool32 messageActive,
                                           bool32 timeoutReached, bool32 headless);

bool32 IsBattleTextReadingEligible(u32 battleTypeFlags, bool32 headless);
void ResetBattleTextReading(void);
void BeginBattleTextReading(void);
void UpdateBattleTextReading(void);
bool32 TryAdvanceBattleTextReading(bool32 presentationBusy);
void FinishBattleTextReading(void);
bool32 IsBattleTextReadingInputUsed(void);
bool32 IsBattleTextReadingHeadless(void);
#if TESTING
void SetBattleTextReadingVisibleForTesting(bool32 visible);
#endif

#endif // GUARD_BATTLE_PRESENTATION_H
