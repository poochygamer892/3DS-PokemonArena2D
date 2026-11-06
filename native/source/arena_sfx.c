#include <3ds.h>
#include <math.h>
#include <string.h>
#include "arena_sfx.h"

/* Procedural effects adapted for the native port from the Korean project's
   WebAudio approach. There are no copied audio files and no music. */
#define SFX_RATE 22050
#define SFX_SAMPLES 4096

static ndspWaveBuf gWaveBuf;
static s16 *gLinearBuffer;
static bool gReady;

bool sfxInit(void)
{
    if (R_FAILED(ndspInit())) return false;
    gLinearBuffer = linearAlloc(SFX_SAMPLES * sizeof(s16));
    if (!gLinearBuffer) { ndspExit(); return false; }
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(0);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, SFX_RATE);
    ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);
    gReady = true;
    return true;
}

void sfxPlay(SfxId id)
{
    if (!gReady || id >= SFX_COUNT) return;
    float start = 260.0f + (float)id * 73.0f;
    float end = (id == SFX_DASH || id == SFX_MOVE) ? start * 2.1f : start * 0.38f;
    if (id == SFX_FIRE) { start = 430.0f; end = 85.0f; }
    if (id == SFX_HEAL) { start = 330.0f; end = 980.0f; }
    for (int i = 0; i < SFX_SAMPLES; ++i) {
        float t = (float)i / (float)SFX_SAMPLES;
        float frequency = start + (end - start) * t;
        float envelope = (1.0f - t) * (1.0f - t);
        float wave = sinf(6.2831853f * frequency * ((float)i / SFX_RATE));
        gLinearBuffer[i] = (s16)(wave * envelope * 7000.0f);
    }
    ndspChnWaveBufClear(0);
    memset(&gWaveBuf, 0, sizeof(gWaveBuf));
    gWaveBuf.data_vaddr = gLinearBuffer;
    gWaveBuf.nsamples = SFX_SAMPLES;
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
