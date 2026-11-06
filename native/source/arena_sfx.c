#include <3ds.h>
#include <string.h>
#include "arena_sfx.h"

typedef struct {
    const u8 *start;
    const u8 *end;
} SfxAsset;

#define SFX_BUFFER_BYTES 49152

extern const u8 sfx_ui_select_start[], sfx_ui_select_end[];
extern const u8 sfx_ui_back_start[], sfx_ui_back_end[];
extern const u8 sfx_ui_confirm_start[], sfx_ui_confirm_end[];
extern const u8 sfx_basic_start[], sfx_basic_end[];
extern const u8 sfx_heavy_hit_start[], sfx_heavy_hit_end[];
extern const u8 sfx_dash_start[], sfx_dash_end[];
extern const u8 sfx_move_start[], sfx_move_end[];
extern const u8 sfx_fire_start[], sfx_fire_end[];
extern const u8 sfx_ko_start[], sfx_ko_end[];
extern const u8 sfx_switch_start[], sfx_switch_end[];
extern const u8 sfx_heal_start[], sfx_heal_end[];

__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global sfx_ui_select_start\n"
    "sfx_ui_select_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/ui-clic.pcm\"\n"
    ".global sfx_ui_select_end\n"
    "sfx_ui_select_end:\n"
    ".global sfx_ui_back_start\n"
    "sfx_ui_back_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/ui-retour.pcm\"\n"
    ".global sfx_ui_back_end\n"
    "sfx_ui_back_end:\n"
    ".global sfx_ui_confirm_start\n"
    "sfx_ui_confirm_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/ui-valider.pcm\"\n"
    ".global sfx_ui_confirm_end\n"
    "sfx_ui_confirm_end:\n"
    ".global sfx_basic_start\n"
    "sfx_basic_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/coup-1.pcm\"\n"
    ".global sfx_basic_end\n"
    "sfx_basic_end:\n"
    ".global sfx_heavy_hit_start\n"
    "sfx_heavy_hit_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/coup-lourd-1.pcm\"\n"
    ".global sfx_heavy_hit_end\n"
    "sfx_heavy_hit_end:\n"
    ".global sfx_dash_start\n"
    "sfx_dash_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/dash-1.pcm\"\n"
    ".global sfx_dash_end\n"
    "sfx_dash_end:\n"
    ".global sfx_move_start\n"
    "sfx_move_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/sort-1.pcm\"\n"
    ".global sfx_move_end\n"
    "sfx_move_end:\n"
    ".global sfx_fire_start\n"
    "sfx_fire_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/feu-1.pcm\"\n"
    ".global sfx_fire_end\n"
    "sfx_fire_end:\n"
    ".global sfx_ko_start\n"
    "sfx_ko_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/ko-choc.pcm\"\n"
    ".global sfx_ko_end\n"
    "sfx_ko_end:\n"
    ".global sfx_switch_start\n"
    "sfx_switch_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/echange.pcm\"\n"
    ".global sfx_switch_end\n"
    "sfx_switch_end:\n"
    ".global sfx_heal_start\n"
    "sfx_heal_start:\n"
    ".incbin \"../assets/arena2d_authorized/sfx/soin.pcm\"\n"
    ".global sfx_heal_end\n"
    "sfx_heal_end:\n"
    ".text\n"
);

static const SfxAsset kSfx[SFX_COUNT] = {
    { sfx_ui_select_start, sfx_ui_select_end },
    { sfx_ui_back_start, sfx_ui_back_end },
    { sfx_ui_confirm_start, sfx_ui_confirm_end },
    { sfx_basic_start, sfx_basic_end },
    { sfx_heavy_hit_start, sfx_heavy_hit_end },
    { sfx_dash_start, sfx_dash_end },
    { sfx_move_start, sfx_move_end },
    { sfx_fire_start, sfx_fire_end },
    { sfx_ko_start, sfx_ko_end },
    { sfx_switch_start, sfx_switch_end },
    { sfx_heal_start, sfx_heal_end },
};

static ndspWaveBuf gWaveBuf;
static u8 *gLinearBuffer;
static bool gReady;

bool sfxInit(void)
{
    if (R_FAILED(ndspInit())) return false;
    gLinearBuffer = linearAlloc(SFX_BUFFER_BYTES);
    if (!gLinearBuffer) {
        ndspExit();
        return false;
    }
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(0);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, 22050.0f);
    ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);
    gReady = true;
    return true;
}

void sfxPlay(SfxId id)
{
    if (!gReady || id < 0 || id >= SFX_COUNT) return;
    size_t bytes = (size_t)(kSfx[id].end - kSfx[id].start);
    if (!bytes || bytes > SFX_BUFFER_BYTES) return;
    ndspChnWaveBufClear(0);
    memset(&gWaveBuf, 0, sizeof(gWaveBuf));
    memcpy(gLinearBuffer, kSfx[id].start, bytes);
    gWaveBuf.data_vaddr = gLinearBuffer;
    gWaveBuf.nsamples = bytes / sizeof(s16);
    ndspChnWaveBufAdd(0, &gWaveBuf);
}

void sfxExit(void)
{
    if (!gReady) return;
    ndspChnWaveBufClear(0);
    linearFree(gLinearBuffer);
    gLinearBuffer = NULL;
    gReady = false;
    ndspExit();
}
