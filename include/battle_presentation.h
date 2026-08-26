#ifndef GUARD_BATTLE_PRESENTATION_H
#define GUARD_BATTLE_PRESENTATION_H

bool32 ShouldAdvanceBattlePresentationWait(u32 battleTypeFlags, u16 newKeys,
                                           bool32 controllerBusy, bool32 textPrinterActive,
                                           bool32 soundActive, bool32 messageActive,
                                           bool32 timeoutReached, bool32 headless);

#endif // GUARD_BATTLE_PRESENTATION_H
