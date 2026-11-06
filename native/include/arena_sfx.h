#pragma once

#include <stdbool.h>

typedef enum {
    SFX_UI_SELECT,
    SFX_UI_BACK,
    SFX_UI_CONFIRM,
    SFX_BASIC,
    SFX_HEAVY_HIT,
    SFX_DASH,
    SFX_MOVE,
    SFX_FIRE,
    SFX_KO,
    SFX_SWITCH,
    SFX_HEAL,
    SFX_COUNT
} SfxId;

bool sfxInit(void);
void sfxPlay(SfxId id);
void sfxExit(void);
