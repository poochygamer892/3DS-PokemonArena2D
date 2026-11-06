#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "arena_sfx.h"
#include "web_arena_data.h"
#include "poke2d_asset_archive.h"
#include "poke2d_font_indices.h"
#include "poke2d_sprite_meta.h"

#define TEAM_SIZE 1
#define ARENA_COUNT 4
#define TOP_W 400.0f
#define TOP_H 240.0f
#define BOTTOM_W 320.0f
#define BOTTOM_H 240.0f

typedef enum { SCENE_TITLE, SCENE_BATTLE, SCENE_PAUSE, SCENE_RESULT } Scene;
typedef enum { DROP_HEAL, DROP_CHARGE, DROP_SPEED } DropKind;
typedef enum { MODE_QUICK, MODE_CPU, MODE_STORY, MODE_RUN, MODE_TRAINING } GameMode;
typedef enum {
    PAGE_HOME, PAGE_PLAY, PAGE_CPU, PAGE_STORY, PAGE_RUN, PAGE_TEAMS,
    PAGE_ONLINE, PAGE_SETTINGS, PAGE_DEX, PAGE_TUTORIAL, PAGE_CREDITS,
    PAGE_2PLAY
} MenuPage;

typedef struct {
    const char *name;
    char type[24];
    int speciesArt;
    int rosterIndex;
    int formIndex;
    int evolutionStage;
    float experience;
    int isMega;
    int maxHp;
    int hp;
} Pokemon;

typedef struct {
    float x, y;
    float moveX, moveY;
    float knockbackX, knockbackY;
    float basicCd;
    float skillCd[4];
    float dashCd;
    float dashIFrames;
    float switchCd;
    float speedBoost;
    float energy;
    float ultimate;
    float hurtFlash;
    float hitStun;
    float burnTime, burnTick;
    float slowTime;
    float comboTimer;
    float attackAnim;
    float animClock;
    int comboCount;
    int animKind;
} Combatant;

typedef struct {
    float x, y, vx, vy, life;
    int damage;
    int attackType;
    int status;
    float statusTime;
    float knockback;
    int active;
    int side;
    u32 color;
} Shot;

typedef enum { MOVE_MELEE, MOVE_LINE, MOVE_BURST, MOVE_SPREAD, MOVE_DASH } MoveStyle;
typedef enum { TYPE_NEUTRAL, TYPE_GRASS, TYPE_FIRE, TYPE_WATER, TYPE_ELECTRIC } AttackType;
typedef enum { STATUS_NONE, STATUS_BURN, STATUS_SLOW, STATUS_STUN, STATUS_DRAIN } StatusKind;
typedef enum { FX_SLASH, FX_BURST, FX_DODGE, FX_BEAM } EffectKind;

typedef struct {
    MoveStyle style;
    int damage;
    float cooldown;
    float range;
    AttackType type;
    StatusKind status;
    float statusTime;
    float energy;
    float velocity;
    u32 color;
} MoveDefinition;

typedef struct {
    float x, y, endX, endY;
    float life, maxLife;
    EffectKind kind;
    u32 color;
} AttackEffect;

static u32 moveColorForType(AttackType type);

static Pokemon gTeam[2][TEAM_SIZE];
static const char *const kArenaNames[ARENA_COUNT] = {
    "STADIUM", "NEON", "CRATER", "RUINS"
};

enum { ANIM_IDLE, ANIM_WALK, ANIM_SHOOT };

static C3D_RenderTarget *gTop;
static C3D_RenderTarget *gBottom;
static C2D_SpriteSheet gIconAtlas;
static C2D_SpriteSheet gFighterAtlases[2][P2D_ANIM_COUNT];
static int gLoadedFighterSpecies[2] = { -1, -1 };
static C2D_SpriteSheet gFontAtlas;
static C2D_SpriteSheet gMenuSheet;
static C2D_SpriteSheet gTitleSheet;
/* The Old 2DS has a small texture cache.  Keeping four 400x240 RGB arenas
 * resident made Azahar's software renderer thrash and dropped battles to
 * roughly 11 FPS.  Only the selected web arena needs to be in VRAM. */
static C2D_SpriteSheet gArenaSheet;
static C2D_Image gMenuBackground;
static C2D_Image gArenaImage;
static int gLoadedArenaIndex = -1;
static C2D_Image gWebTitle;
static C2D_Image gGameLogo;
static C2D_Image gTextFill[P2D_TEXT_COUNT];
static Scene gScene = SCENE_TITLE;
static MenuPage gMenuPage = PAGE_HOME;
static GameMode gGameMode = MODE_QUICK;
static Combatant gFighter[2];
static Shot gShots[8];
static AttackEffect gEffects[16];
static int gActive[2] = { 0, 0 };
static int gRoundWins[2] = { 0, 0 };
static int gRoundNumber = 1;
static int gWinner = -1;
static int gMenuSelection = 0;
static MenuPage gMenuStack[4];
static int gMenuDepth = 0;
static int gStoryStage = 0;
static int gStoryUnlocked = 0;
static int gRunFloor = 1;
static int gCpuDifficulty = 1;
static int gArenaIndex = 0;
static int gPlayerSpecies[TEAM_SIZE] = { 0 };
static int gOpponentSpecies = 1;
static float gUiTextScale = 1.0f;
static float gBattleTime = 0.0f;
static float gCpuBasicCd = 0.0f;
static float gCpuSkillCd = 2.0f;
static float gSupplyIn = 10.0f;
static float gSupplyX = 0.0f;
static float gSupplyY = 0.0f;
static float gSupplyLife = 0.0f;
static int gSupplyKind = DROP_HEAL;
static int gSupplyCount = 0;
static float gHitFlash = 0.0f;
static char gHitLabel[24] = "HIT!";
static u32 gMenuStickDirection = 0;
static float gMenuStickRepeat = 0.0f;

static const u32 C_BG = 0x142330FF;
static const u32 C_TEXT = 0xF8F3D8FF;
static const u32 C_MUTED = 0xA7B2CBFF;
static const u32 C_GREEN = 0x57CB69FF;
static const u32 C_RED = 0xE75157FF;
static const u32 C_GOLD = 0xFFC53DFF;

extern const unsigned char poke2d_icon_atlas_start[];
extern const unsigned char poke2d_icon_atlas_end[];
extern const unsigned char poke2d_font_atlas_start[];
extern const unsigned char poke2d_font_atlas_end[];
extern const unsigned char menu_background_t3x_start[];
extern const unsigned char menu_background_t3x_end[];
extern const unsigned char web_title_t3x_start[];
extern const unsigned char web_title_t3x_end[];
extern const unsigned char arena_stadium_t3x_start[];
extern const unsigned char arena_stadium_t3x_end[];
extern const unsigned char arena_neon_t3x_start[];
extern const unsigned char arena_neon_t3x_end[];
extern const unsigned char arena_crater_t3x_start[];
extern const unsigned char arena_crater_t3x_end[];
extern const unsigned char arena_ruinas_t3x_start[];
extern const unsigned char arena_ruinas_t3x_end[];

__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global poke2d_icon_atlas_start\n"
    "poke2d_icon_atlas_start:\n"
    ".incbin \"../generated/pokearena2d/poke2d_icon_atlas.t3x\"\n"
    ".global poke2d_icon_atlas_end\n"
    "poke2d_icon_atlas_end:\n"
    ".text\n"
);

__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global arena_stadium_t3x_start\n"
    "arena_stadium_t3x_start:\n"
    ".incbin \"../generated/pokearena2d/arena_stadium.t3x\"\n"
    ".global arena_stadium_t3x_end\n"
    "arena_stadium_t3x_end:\n"
    ".text\n"
);

__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global arena_neon_t3x_start\n"
    "arena_neon_t3x_start:\n"
    ".incbin \"../generated/pokearena2d/arena_neon.t3x\"\n"
    ".global arena_neon_t3x_end\n"
    "arena_neon_t3x_end:\n"
    ".text\n"
);

__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global arena_crater_t3x_start\n"
    "arena_crater_t3x_start:\n"
    ".incbin \"../generated/pokearena2d/arena_crater.t3x\"\n"
    ".global arena_crater_t3x_end\n"
    "arena_crater_t3x_end:\n"
    ".text\n"
);

__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global arena_ruinas_t3x_start\n"
    "arena_ruinas_t3x_start:\n"
    ".incbin \"../generated/pokearena2d/arena_ruinas.t3x\"\n"
    ".global arena_ruinas_t3x_end\n"
    "arena_ruinas_t3x_end:\n"
    ".text\n"
);

__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global poke2d_font_atlas_start\n"
    "poke2d_font_atlas_start:\n"
    ".incbin \"../generated/pokearena2d/poke2d_font_atlas.t3x\"\n"
    ".global poke2d_font_atlas_end\n"
    "poke2d_font_atlas_end:\n"
    ".text\n"
);

__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global menu_background_t3x_start\n"
    "menu_background_t3x_start:\n"
    ".incbin \"../generated/pokearena2d/menu_background.t3x\"\n"
    ".global menu_background_t3x_end\n"
    "menu_background_t3x_end:\n"
    ".text\n"
);

__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global web_title_t3x_start\n"
    "web_title_t3x_start:\n"
    ".incbin \"../generated/pokearena2d/web_title.t3x\"\n"
    ".global web_title_t3x_end\n"
    "web_title_t3x_end:\n"
    ".text\n"
);

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float distance2(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

static u32 c2dColor(u32 rgba)
{
    return C2D_Color32((rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF,
                       (rgba >> 8) & 0xFF, rgba & 0xFF);
}

static void rect(float x, float y, float w, float h, u32 color)
{
    C2D_DrawRectSolid(x, y, 0.5f, w, h, c2dColor(color));
}

static float bitmapTextWidth(const char *s, float scale)
{
    float width = 0.0f;
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        if (c == '\n') continue;
        if (c < P2D_TEXT_FIRST || c > 126) c = '?';
        width += (float)p2dTextAdvance[c - P2D_TEXT_FIRST] * scale;
    }
    return width;
}

static void drawBitmapText(float x, float y, float scale, u32 color, const char *s)
{
    /* Open Sans glyphs are rasterized at 40 px and filtered from a separate
       RGBA8 atlas so their thinner strokes remain smooth on the 3DS. */
    float glyphScale = scale * 0.90f * gUiTextScale;
    float startX = x;
    C2D_ImageTint fillTint;
    C2D_PlainImageTint(&fillTint, c2dColor(color), 1.0f);
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        if (c == '\n') {
            x = startX;
            y += 40.0f * glyphScale;
            continue;
        }
        if (c < P2D_TEXT_FIRST || c > 126) c = '?';
        int index = c - P2D_TEXT_FIRST;
        if (c != ' ') {
            C2D_DrawImageAt(gTextFill[index], x, y, 0.61f, &fillTint,
                            glyphScale, glyphScale);
        }
        x += (float)p2dTextAdvance[index] * glyphScale;
    }
}

static void textAt(float x, float y, float scale, u32 color, const char *s)
{
    drawBitmapText(x, y, scale, color, s);
}

static void centeredText(float x, float y, float scale, u32 color, const char *s)
{
    float actualScale = scale * 0.90f * gUiTextScale;
    drawBitmapText(x - bitmapTextWidth(s, actualScale) * 0.5f,
                   y, scale, color, s);
}

static bool loadArenaTexture(int arena)
{
    if (arena < 0 || arena >= ARENA_COUNT) arena = 0;
    if (gArenaSheet && gLoadedArenaIndex == arena) return true;
    if (gArenaSheet) {
        C2D_SpriteSheetFree(gArenaSheet);
        gArenaSheet = NULL;
    }
    const unsigned char *start = arena_stadium_t3x_start;
    const unsigned char *end = arena_stadium_t3x_end;
    switch (arena) {
    case 1: start = arena_neon_t3x_start; end = arena_neon_t3x_end; break;
    case 2: start = arena_crater_t3x_start; end = arena_crater_t3x_end; break;
    case 3: start = arena_ruinas_t3x_start; end = arena_ruinas_t3x_end; break;
    default: break;
    }
    gArenaSheet = C2D_SpriteSheetLoadFromMem(start, (size_t)(end - start));
    if (!gArenaSheet) {
        gLoadedArenaIndex = -1;
        return false;
    }
    gArenaImage = C2D_SpriteSheetGetImage(gArenaSheet, 0);
    C3D_TexSetFilter(gArenaImage.tex, GPU_LINEAR, GPU_LINEAR);
    gLoadedArenaIndex = arena;
    return true;
}

static void fittedText(float x, float y, float maxWidth, float scale,
                       u32 color, const char *s, bool centered)
{
    float actualScale = scale * 0.90f * gUiTextScale;
    float width = bitmapTextWidth(s, actualScale);
    if (maxWidth > 0.0f && width > maxWidth) {
        scale *= maxWidth / width;
        actualScale *= maxWidth / width;
        width = maxWidth;
    }
    if (centered) x -= width * 0.5f;
    drawBitmapText(x, y, scale, color, s);
}

static void drawBar(float x, float y, float w, float h, int hp, int maxHp)
{
    float ratio = maxHp > 0 ? clampf((float)hp / (float)maxHp, 0.0f, 1.0f) : 0.0f;
    rect(x, y, w, h, 0x101721FF);
    if (ratio > 0.0f)
        rect(x + 1.0f, y + 1.0f, (w - 2.0f) * ratio, h - 2.0f, ratio > 0.35f ? C_GREEN : C_RED);
}

static bool ensureFighterAtlas(int side, int rosterIndex, int animation)
{
    if (side < 0 || side > 1 || rosterIndex < 0 ||
        rosterIndex >= ARENA2D_ROSTER_COUNT) return false;
    if (animation < 0 || animation >= P2D_ANIM_COUNT) return false;
    if (gFighterAtlases[side][animation] && gLoadedFighterSpecies[side] == rosterIndex)
        return true;
    if (gLoadedFighterSpecies[side] != rosterIndex) {
        for (int i = 0; i < P2D_ANIM_COUNT; ++i) {
            if (gFighterAtlases[side][i]) C2D_SpriteSheetFree(gFighterAtlases[side][i]);
            gFighterAtlases[side][i] = NULL;
        }
        gLoadedFighterSpecies[side] = rosterIndex;
    }
    const P2dEmbeddedAtlas *asset = &p2dFighterAtlasData[rosterIndex][animation];
    size_t size = (size_t)(asset->end - asset->start);
    if (size == 0) return false;
    gFighterAtlases[side][animation] = C2D_SpriteSheetLoadFromMem(asset->start, size);
    if (!gFighterAtlases[side][animation]) return false;
    return true;
}

static void freeFighterAtlases(void)
{
    for (int side = 0; side < 2; ++side) {
        for (int animation = 0; animation < P2D_ANIM_COUNT; ++animation)
            if (gFighterAtlases[side][animation])
                C2D_SpriteSheetFree(gFighterAtlases[side][animation]);
        gLoadedFighterSpecies[side] = -1;
    }
}

static C2D_Image pokemonFrame(int side, int rosterIndex, int formIndex,
                              int animation, int direction, int frame)
{
    if (formIndex < 0 || formIndex >= ARENA2D_FORM_COUNT) formIndex = 0;
    int art = kArena2dForms[formIndex].spriteArt;
    if (art < 0 || art >= P2D_SPECIES_COUNT) art = 0;
    if (animation < 0 || animation >= P2D_ANIM_COUNT) animation = ANIM_IDLE;
    if (direction < 0 || direction >= P2D_DIR_COUNT) direction = 0;
    if (!ensureFighterAtlas(side, rosterIndex, animation)) return (C2D_Image){ 0 };
    int count = p2dFrameCount[art][animation];
    if (count < 1) count = 1;
    if (frame < 0) frame = 0;
    frame %= count;
    C2D_Image image = C2D_SpriteSheetGetImage(gFighterAtlases[side][animation],
        p2dSpriteIndex[formIndex][animation][direction][frame]);
    C3D_TexSetFilter(image.tex, GPU_NEAREST, GPU_NEAREST);
    return image;
}

static void drawPokemonAt(int side, int rosterIndex, int art, int formIndex,
                          int animation, int direction, int frame,
                          float pivotX, float pivotY, float formScale)
{
    C2D_Image image = pokemonFrame(side, rosterIndex, formIndex,
                                   animation, direction, frame);
    if (!image.tex) return;
    /* Frames are kept at their original web pixel resolution. */
    float scale = p2dSpeciesScale[art] * formScale;
    float originX = (float)p2dOriginX[art][animation] * scale;
    float originY = (float)p2dOriginY[art][animation] * scale;
    C2D_DrawImageAt(image, pivotX - originX, pivotY - originY, 0.36f,
                    NULL, scale, scale);
}

static void drawPokemonIcon(int rosterIndex, float x, float y, float height)
{
    if (rosterIndex < 0 || rosterIndex >= ARENA2D_ROSTER_COUNT) rosterIndex = 0;
    int art = kArena2dRoster[rosterIndex].fallbackArt;
    C2D_Image image = C2D_SpriteSheetGetImage(gIconAtlas, p2dIconIndex[rosterIndex]);
    C3D_TexSetFilter(image.tex, GPU_NEAREST, GPU_NEAREST);
    float sourceHeight = (float)p2dFrameHeight[art][ANIM_IDLE];
    float scale = sourceHeight > 0.0f ? height / sourceHeight : 1.0f;
    C2D_DrawImageAt(image, x, y, 0.18f, NULL, scale, scale);
}

static void drawWebUiIcon(int icon, float x, float y, float height)
{
    static const unsigned short indices[] = {
        poke2d_icon_indices_ui_icon_00_idx, poke2d_icon_indices_ui_icon_01_idx,
        poke2d_icon_indices_ui_icon_02_idx, poke2d_icon_indices_ui_icon_03_idx,
        poke2d_icon_indices_ui_icon_04_idx, poke2d_icon_indices_ui_icon_05_idx,
        poke2d_icon_indices_ui_icon_06_idx
    };
    if (icon < 0 || icon >= (int)(sizeof(indices) / sizeof(indices[0]))) return;
    C2D_Image image = C2D_SpriteSheetGetImage(gIconAtlas, indices[icon]);
    C3D_TexSetFilter(image.tex, GPU_NEAREST, GPU_NEAREST);
    float scale = height / 108.0f;
    C2D_DrawImageAt(image, x, y, 0.18f, NULL, scale, scale);
}

static void drawStage(C2D_Image image)
{
    C2D_DrawImageAt(image, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
}

static int faceDirection(float dx, float dy)
{
    // Poké Arena 2D sheets use rows down, down-right, right, up-right, up,
    // up-left, left, down-left.
    float angle = atan2f(dy, dx);
    int sector = (int)floorf(angle / 0.78539816339f + 0.5f);
    int row = (2 - sector) % 8;
    return row < 0 ? row + 8 : row;
}

static Pokemon pokemonFromRoster(int rosterIndex)
{
    if (rosterIndex < 0 || rosterIndex >= ARENA2D_ROSTER_COUNT)
        rosterIndex = 0;
    const Arena2dRosterEntry *entry = &kArena2dRoster[rosterIndex];
    Pokemon pokemon = { 0 };
    pokemon.rosterIndex = rosterIndex;
    pokemon.formIndex = entry->baseForm;
    const Arena2dFormEntry *form = &kArena2dForms[pokemon.formIndex];
    pokemon.name = form->name;
    if (form->type2[0])
        snprintf(pokemon.type, sizeof(pokemon.type), "%s / %s", form->type1,
                 form->type2);
    else
        snprintf(pokemon.type, sizeof(pokemon.type), "%s", form->type1);
    pokemon.speciesArt = form->spriteArt;
    pokemon.maxHp = (int)ceilf(form->base[0] * 1.8f);
    pokemon.hp = pokemon.maxHp;
    return pokemon;
}

static void setPokemonForm(Pokemon *pokemon, int formIndex)
{
    if (!pokemon || formIndex < 0 || formIndex >= ARENA2D_FORM_COUNT) return;
    const Arena2dFormEntry *form = &kArena2dForms[formIndex];
    pokemon->formIndex = formIndex;
    pokemon->name = form->name;
    if (form->type2[0])
        snprintf(pokemon->type, sizeof(pokemon->type), "%s / %s", form->type1,
                 form->type2);
    else
        snprintf(pokemon->type, sizeof(pokemon->type), "%s", form->type1);
    pokemon->speciesArt = form->spriteArt;
    pokemon->maxHp = (int)ceilf(form->base[0] * 1.8f);
}

static const Arena2dMoveEntry *rosterMove(const Pokemon *pokemon, int slot)
{
    if (!pokemon || pokemon->rosterIndex < 0 ||
        pokemon->rosterIndex >= ARENA2D_ROSTER_COUNT || slot < 0 || slot >= 4)
        return NULL;
    return &kArena2dMoves[kArena2dForms[pokemon->formIndex].moves[slot + 1]];
}

static const Arena2dMoveEntry *rosterBasicMove(const Pokemon *pokemon)
{
    if (!pokemon || pokemon->rosterIndex < 0 ||
        pokemon->rosterIndex >= ARENA2D_ROSTER_COUNT) return NULL;
    return &kArena2dMoves[kArena2dForms[pokemon->formIndex].moves[0]];
}

static bool transformPokemon(int side)
{
    Pokemon *pokemon = &gTeam[side][gActive[side]];
    const Arena2dRosterEntry *line = &kArena2dRoster[pokemon->rosterIndex];
    Combatant *fighter = &gFighter[side];
    if (pokemon->evolutionStage < line->evolutionCount) {
        int stage = pokemon->evolutionStage;
        float threshold = line->evolutionThreshold[stage];
        if (pokemon->experience >= threshold && threshold > 0.0f) {
            float healthRatio = pokemon->maxHp > 0 ?
                (float)pokemon->hp / (float)pokemon->maxHp : 1.0f;
            pokemon->experience -= threshold;
            pokemon->evolutionStage++;
            pokemon->isMega = 0;
            setPokemonForm(pokemon, line->evolutionForms[stage]);
            float healedRatio = healthRatio + line->evolutionHeal * (1.0f - healthRatio);
            pokemon->hp = (int)ceilf((float)pokemon->maxHp * healedRatio);
            if (pokemon->hp > pokemon->maxHp) pokemon->hp = pokemon->maxHp;
            fighter->dashIFrames = 0.9f;
            fighter->hitStun = 0.0f;
            fighter->burnTime = fighter->slowTime = 0.0f;
            snprintf(gHitLabel, sizeof(gHitLabel), "EVOLVED!");
            gHitFlash = 1.0f;
            sfxPlay(SFX_MOVE);
            return true;
        }
    }
    int megaForm = line->megaForms[pokemon->evolutionStage];
    if (!pokemon->isMega && megaForm >= 0 && fighter->ultimate >= 100.0f) {
        setPokemonForm(pokemon, megaForm);
        pokemon->isMega = 1;
        fighter->ultimate = 0.0f;
        fighter->dashIFrames = 0.9f;
        snprintf(gHitLabel, sizeof(gHitLabel), "MEGA EVOLUTION!");
        gHitFlash = 1.0f;
        sfxPlay(SFX_HEAVY_HIT);
        return true;
    }
    return false;
}

static void resetBattle(void)
{
    int retainedHp[TEAM_SIZE] = { 0 };
    int playerStage = gTeam[0][0].evolutionStage;
    float playerExperience = gTeam[0][0].experience;
    int cpuStage = gTeam[1][0].evolutionStage;
    float cpuExperience = gTeam[1][0].experience;
    bool carryRunHp = gGameMode == MODE_RUN && gRunFloor > 1 &&
                      gScene == SCENE_RESULT && gWinner == 0;
    if (carryRunHp)
        for (int i = 0; i < TEAM_SIZE; ++i)
            retainedHp[i] = gTeam[0][i].hp;
    gTeam[0][0] = pokemonFromRoster(gPlayerSpecies[0]);
    gTeam[1][0] = pokemonFromRoster(gOpponentSpecies);
    if (gRoundNumber > 1) {
        int stage = playerStage < kArena2dRoster[gPlayerSpecies[0]].evolutionCount ?
                    playerStage : kArena2dRoster[gPlayerSpecies[0]].evolutionCount;
        if (stage > 0) {
            gTeam[0][0].evolutionStage = stage;
            gTeam[0][0].experience = playerExperience;
            setPokemonForm(&gTeam[0][0], kArena2dRoster[gPlayerSpecies[0]].evolutionForms[stage - 1]);
        }
        stage = cpuStage < kArena2dRoster[gOpponentSpecies].evolutionCount ?
                cpuStage : kArena2dRoster[gOpponentSpecies].evolutionCount;
        if (stage > 0) {
            gTeam[1][0].evolutionStage = stage;
            gTeam[1][0].experience = cpuExperience;
            setPokemonForm(&gTeam[1][0], kArena2dRoster[gOpponentSpecies].evolutionForms[stage - 1]);
        }
    }
    gTeam[0][0].hp = gTeam[0][0].maxHp;
    gTeam[1][0].hp = gTeam[1][0].maxHp;
    if (carryRunHp)
        gTeam[0][0].hp = retainedHp[0] < gTeam[0][0].maxHp ?
                         retainedHp[0] : gTeam[0][0].maxHp;
    if (gGameMode == MODE_STORY) {
        gTeam[1][0] = pokemonFromRoster((gStoryStage + 1) % ARENA2D_ROSTER_COUNT);
        gTeam[1][0].hp = gTeam[1][0].maxHp;
        for (int i = 1; i < TEAM_SIZE; ++i) {
            gTeam[0][i].hp = 0;
            gTeam[1][i].hp = 0;
        }
    }

    gActive[0] = 0;
    gActive[1] = 0;
    memset(gFighter, 0, sizeof(gFighter));
    memset(gShots, 0, sizeof(gShots));
    memset(gEffects, 0, sizeof(gEffects));
    gFighter[0].x = 92.0f;
    gFighter[0].y = 155.0f;
    gFighter[1].x = 308.0f;
    gFighter[1].y = 130.0f;
    gFighter[0].energy = 30.0f;
    gFighter[1].energy = 30.0f;
    gFighter[0].ultimate = 0.0f;
    gFighter[1].ultimate = 0.0f;
    gBattleTime = 99.0f;
    static const float basicDelay[3] = { 1.10f, 0.90f, 0.67f };
    static const float skillDelay[3] = { 4.70f, 3.40f, 2.45f };
    gCpuBasicCd = basicDelay[gCpuDifficulty];
    gCpuSkillCd = skillDelay[gCpuDifficulty];
    gSupplyIn = 10.0f;
    gSupplyLife = 0.0f;
    gSupplyCount = 0;
    gHitFlash = 0.0f;
    snprintf(gHitLabel, sizeof(gHitLabel), "HIT!");
    gWinner = -1;
    gScene = SCENE_BATTLE;
}

static int nextLiving(int side, int from)
{
    for (int step = 1; step <= TEAM_SIZE; ++step) {
        int candidate = (from + step) % TEAM_SIZE;
        if (gTeam[side][candidate].hp > 0)
            return candidate;
    }
    return -1;
}

static int livingCount(int side)
{
    int alive = 0;
    for (int i = 0; i < TEAM_SIZE; ++i)
        if (gTeam[side][i].hp > 0) ++alive;
    return alive;
}

static void switchPokemon(int side, int slot)
{
    if (slot < 0 || slot >= TEAM_SIZE || gFighter[side].switchCd > 0.0f)
        return;
    if (slot == gActive[side] || gTeam[side][slot].hp <= 0)
        return;

    gActive[side] = slot;
    sfxPlay(SFX_SWITCH);
    gFighter[side].switchCd = 3.5f;
    gFighter[side].dashIFrames = 0.2f;
    gFighter[side].burnTime = 0.0f;
    gFighter[side].burnTick = 0.0f;
    gFighter[side].slowTime = 0.0f;
    gFighter[side].hitStun = 0.0f;
    gFighter[side].knockbackX = 0.0f;
    gFighter[side].knockbackY = 0.0f;
    gFighter[side].comboCount = 0;
    gFighter[side].comboTimer = 0.0f;
    if (side == 0) {
        gFighter[0].x = clampf(gFighter[0].x - 20.0f, 35.0f, 365.0f);
    } else {
        gFighter[1].x = clampf(gFighter[1].x + 20.0f, 35.0f, 365.0f);
    }
}

static void damagePokemon(int side, int damage, float chargeRatio)
{
    if (gFighter[side].dashIFrames > 0.0f)
        return;
    Pokemon *p = &gTeam[side][gActive[side]];
    if (p->hp <= 0) return;
    p->hp -= damage;
    if (p->hp < 0) p->hp = 0;
    gFighter[side].hurtFlash = 0.16f;
    gHitFlash = 0.12f;
    int opponent = side == 0 ? 1 : 0;
    int oppMax = gTeam[opponent][gActive[opponent]].maxHp;
    if (oppMax > 0)
        gFighter[opponent].ultimate = clampf(gFighter[opponent].ultimate + (float)damage / (float)oppMax * chargeRatio, 0.0f, 100.0f);

    if (p->hp == 0) {
        if (gGameMode == MODE_TRAINING && side == 1) {
            p->hp = p->maxHp;
            gFighter[side].dashIFrames = 0.45f;
            return;
        }
        sfxPlay(SFX_KO);
        int next = nextLiving(side, gActive[side]);
        if (next >= 0) {
            gActive[side] = next;
            gFighter[side].switchCd = 0.0f;
            gFighter[side].dashIFrames = 0.78f;
            gFighter[side].burnTime = 0.0f;
            gFighter[side].burnTick = 0.0f;
            gFighter[side].slowTime = 0.0f;
            gFighter[side].hitStun = 0.0f;
            gFighter[side].knockbackX = 0.0f;
            gFighter[side].knockbackY = 0.0f;
            gFighter[side].comboCount = 0;
            gFighter[side].comboTimer = 0.0f;
        }
    }
}

static void playAttackAnimation(int side)
{
    gFighter[side].attackAnim = 0.44f;
    gFighter[side].animClock = 0.0f;
    gFighter[side].animKind = ANIM_SHOOT;
}

static AttackEffect *spawnEffect(EffectKind kind, float x, float y,
                                 float endX, float endY, float life, u32 color)
{
    for (int i = 0; i < 16; ++i) {
        AttackEffect *effect = &gEffects[i];
        if (effect->life > 0.0f) continue;
        effect->x = x;
        effect->y = y;
        effect->endX = endX;
        effect->endY = endY;
        effect->life = effect->maxLife = life;
        effect->kind = kind;
        effect->color = color;
        return effect;
    }
    return NULL;
}

static AttackType speciesType(int species)
{
    switch (species) {
    case 0: return TYPE_GRASS;
    case 1: return TYPE_FIRE;
    case 2: return TYPE_WATER;
    case 3: return TYPE_ELECTRIC;
    case 4: return TYPE_FIRE;
    case 5: return TYPE_WATER;
    default: return TYPE_NEUTRAL;
    }
}

static AttackType attackTypeFromName(const char *type)
{
    if (!type) return TYPE_NEUTRAL;
    if (strcmp(type, "FIRE") == 0) return TYPE_FIRE;
    if (strcmp(type, "WATER") == 0 || strcmp(type, "ICE") == 0) return TYPE_WATER;
    if (strcmp(type, "GRASS") == 0 || strcmp(type, "BUG") == 0) return TYPE_GRASS;
    if (strcmp(type, "ELECTRIC") == 0) return TYPE_ELECTRIC;
    return TYPE_NEUTRAL;
}

static MoveDefinition moveFromEntry(const Arena2dMoveEntry *source)
{
    MoveDefinition move = { MOVE_LINE, 1, 3.0f, 220.0f,
                            TYPE_NEUTRAL, STATUS_NONE, 0.0f, 0.0f, 0.0f, 0xD9C5A2FF };
    if (!source) return move;
    move.type = attackTypeFromName(source->type);
    move.color = moveColorForType(move.type);
    move.damage = source->power > 0.0f ? (int)ceilf(source->power) : 0;
    move.cooldown = source->cooldown > 0.0f ? source->cooldown : 4.0f;
    move.range = source->range > 0.0f ? clampf(source->range * 0.44f, 20.0f, 250.0f) : 80.0f;
    move.energy = source->energy;
    move.velocity = source->velocity * 0.44f;
    if (strcmp(source->kind, "tajo") == 0 || strcmp(source->kind, "golpe") == 0 ||
        strcmp(source->kind, "lanzamiento") == 0 || strcmp(source->kind, "contra") == 0)
        move.style = MOVE_MELEE;
    else if (strcmp(source->kind, "dash") == 0 || strcmp(source->kind, "embestidas") == 0 ||
             strcmp(source->kind, "patada") == 0 || strcmp(source->kind, "vuelo") == 0 ||
             strcmp(source->kind, "salto") == 0 || strcmp(source->kind, "ascenso") == 0)
        move.style = MOVE_DASH;
    else if (strcmp(source->kind, "cono") == 0 || strcmp(source->kind, "burbujas") == 0 ||
             strcmp(source->kind, "abanico") == 0 || strcmp(source->kind, "lluvia") == 0 ||
             strcmp(source->kind, "bolaFuego") == 0 || strcmp(source->kind, "bolaShuriken") == 0)
        move.style = MOVE_SPREAD;
    else if (strcmp(source->kind, "pulso") == 0 || strcmp(source->kind, "viento") == 0 ||
             strcmp(source->kind, "caparazon") == 0 || strcmp(source->kind, "chorro") == 0 ||
             strcmp(source->kind, "erupcion") == 0 || strcmp(source->kind, "tormentaHojas") == 0 ||
             strcmp(source->kind, "rafaga") == 0 || strcmp(source->kind, "andanada") == 0 ||
             strcmp(source->kind, "maleficio") == 0 || strcmp(source->kind, "meteoro") == 0 ||
             strcmp(source->kind, "torbellino") == 0 || strcmp(source->kind, "trampa") == 0 ||
             strcmp(source->kind, "explosion") == 0 || strcmp(source->kind, "juicio") == 0 ||
             strcmp(source->kind, "frenesi") == 0 || strcmp(source->kind, "tormenta") == 0 ||
             strcmp(source->kind, "estallido") == 0 || strcmp(source->kind, "vcreate") == 0 ||
             strcmp(source->kind, "redElectrica") == 0 || strcmp(source->kind, "bombaLodo") == 0 ||
             strcmp(source->kind, "bolaSombra") == 0) move.style = MOVE_BURST;
    if (strcmp(source->effect, "burn") == 0 || strcmp(source->effect, "poison") == 0) {
        move.status = STATUS_BURN;
        move.statusTime = 3.0f;
    } else if (strcmp(source->effect, "slow") == 0) {
        move.status = STATUS_SLOW;
        move.statusTime = 2.0f;
    } else if (strcmp(source->effect, "stun") == 0 || strcmp(source->effect, "sleep") == 0 ||
               strcmp(source->effect, "confuse") == 0) {
        move.status = STATUS_STUN;
        move.statusTime = 0.6f;
    } else if (strcmp(source->effect, "drain") == 0) {
        move.status = STATUS_DRAIN;
    }
    return move;
}

static u32 moveColorForType(AttackType type)
{
    switch (type) {
    case TYPE_FIRE: return 0xF06A3DFF;
    case TYPE_WATER: return 0x5EADEFFF;
    case TYPE_GRASS: return 0x70C75DFF;
    case TYPE_ELECTRIC: return 0xF3D148FF;
    default: return 0xD9C5A2FF;
    }
}

static MoveDefinition moveFromRoster(const Pokemon *pokemon, int slot)
{
    return moveFromEntry(rosterMove(pokemon, slot));
}

static MoveDefinition moveFromBasic(const Pokemon *pokemon)
{
    return moveFromEntry(rosterBasicMove(pokemon));
}

static float typeMultiplier(AttackType attack, AttackType defender)
{
    /* The web game ships with type-chart damage disabled by default. */
    (void)attack;
    (void)defender;
    return 1.0f;
}

static void combatHit(int attacker, int rawDamage, AttackType attackType,
                      StatusKind status, float statusTime, float knockback,
                      float stun, u32 color)
{
    int target = attacker == 0 ? 1 : 0;
    Pokemon *attackerPokemon = &gTeam[attacker][gActive[attacker]];
    Pokemon *targetPokemon = &gTeam[target][gActive[target]];
    AttackType defenderType = targetPokemon->rosterIndex >= 0 ?
        attackTypeFromName(kArena2dForms[targetPokemon->formIndex].type1) :
        speciesType(targetPokemon->speciesArt);
    float effectiveness = typeMultiplier(attackType, defenderType);
    float attackPower = kArena2dForms[attackerPokemon->formIndex].base[1];
    float defense = kArena2dForms[targetPokemon->formIndex].base[2];
    float multiplier = effectiveness * attackPower / fmaxf(0.5f, defense);
    int damage = (int)ceilf((float)rawDamage * multiplier);
    if (damage < 1) damage = 1;
    int activeBefore = gActive[target];
    int hpBefore = gTeam[target][activeBefore].hp;
    float dx = gFighter[target].x - gFighter[attacker].x;
    float dy = gFighter[target].y - gFighter[attacker].y;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < 0.1f) length = 1.0f;
    damagePokemon(target, damage, 30.0f);
    if (gActive[target] != activeBefore || gTeam[target][activeBefore].hp == hpBefore)
        return;

    gFighter[attacker].energy = clampf(gFighter[attacker].energy + 6.0f, 0.0f, 100.0f);
    gFighter[target].energy = clampf(gFighter[target].energy + 3.0f, 0.0f, 100.0f);
    if (status == STATUS_DRAIN) {
        int dealt = hpBefore - gTeam[target][activeBefore].hp;
        Pokemon *healer = &gTeam[attacker][gActive[attacker]];
        healer->hp = (int)fminf((float)healer->maxHp,
                                (float)healer->hp + (float)dealt * 0.35f);
    }

    sfxPlay(damage >= 18 ? SFX_HEAVY_HIT : SFX_BASIC);

    Combatant *victim = &gFighter[target];
    victim->knockbackX = dx / length * knockback;
    victim->knockbackY = dy / length * knockback;
    victim->hitStun = fmaxf(victim->hitStun, stun);
    if (status == STATUS_BURN) {
        victim->burnTime = fmaxf(victim->burnTime, statusTime);
        if (victim->burnTick <= 0.0f) victim->burnTick = 0.75f;
    } else if (status == STATUS_SLOW) {
        victim->slowTime = fmaxf(victim->slowTime, statusTime);
    } else if (status == STATUS_STUN) {
        victim->hitStun = fmaxf(victim->hitStun, statusTime);
    }
    spawnEffect(FX_BURST, gFighter[target].x, gFighter[target].y,
                gFighter[target].x, gFighter[target].y, 0.24f, color);
    if (effectiveness > 1.01f)
        snprintf(gHitLabel, sizeof(gHitLabel), "SUPER EFFECTIVE!");
    else if (effectiveness < 0.99f)
        snprintf(gHitLabel, sizeof(gHitLabel), "RESISTED");
    else
        snprintf(gHitLabel, sizeof(gHitLabel), "HIT!");
    gHitFlash = 0.42f;
}

static void fireProjectile(int side, const MoveDefinition *move,
                           float dx, float dy, int damage, float angle)
{
    float cosine = cosf(angle);
    float sine = sinf(angle);
    float speed = move->velocity > 0.0f ? move->velocity : 250.0f;
    float vx = (dx * cosine - dy * sine) * speed;
    float vy = (dx * sine + dy * cosine) * speed;
    for (int i = 0; i < 8; ++i) {
        Shot *shot = &gShots[i];
        if (shot->active) continue;
        shot->x = gFighter[side].x;
        shot->y = gFighter[side].y;
        shot->vx = vx;
        shot->vy = vy;
        shot->life = move->range > 0.0f ? move->range / speed : 1.25f;
        shot->damage = damage;
        shot->attackType = move->type;
        shot->status = move->status;
        shot->statusTime = move->statusTime;
        shot->knockback = 28.0f;
        shot->color = move->color;
        shot->active = 1;
        shot->side = side;
        return;
    }
}

static void useMove(int side, int slot)
{
    int opponent = side == 0 ? 1 : 0;
    Pokemon *activePokemon = &gTeam[side][gActive[side]];
    const Arena2dMoveEntry *sourceMove = slot == -1 ? rosterBasicMove(activePokemon) :
                                                    rosterMove(activePokemon, slot);
    MoveDefinition resolvedMove = slot == -1 ? moveFromBasic(activePokemon) :
                                               moveFromRoster(activePokemon, slot);
    const MoveDefinition *move = &resolvedMove;
    Combatant *fighter = &gFighter[side];
    float startX = fighter->x;
    float startY = fighter->y;
    float dx = gFighter[opponent].x - startX;
    float dy = gFighter[opponent].y - startY;
    float distance = sqrtf(dx * dx + dy * dy);
    if (slot == 3) {
        if (fighter->ultimate < 100.0f) return;
        fighter->ultimate = 0.0f;
    }
    if (move->energy > 0.0f) {
        if (fighter->energy < move->energy) return;
        fighter->energy -= move->energy;
    }
    if (sourceMove && (strcmp(sourceMove->kind, "heal") == 0 ||
                       strcmp(sourceMove->kind, "rest") == 0 ||
                       strcmp(sourceMove->kind, "ring") == 0 ||
                       strcmp(sourceMove->kind, "recuperar") == 0 ||
                       strcmp(sourceMove->kind, "descanso") == 0)) {
        int heal = activePokemon->maxHp *
                   (strcmp(sourceMove->kind, "rest") == 0 ||
                    strcmp(sourceMove->kind, "descanso") == 0 ? 35 : 22) / 100;
        activePokemon->hp = activePokemon->hp + heal > activePokemon->maxHp ?
                            activePokemon->maxHp : activePokemon->hp + heal;
        spawnEffect(FX_BURST, fighter->x, fighter->y, fighter->x, fighter->y,
                    0.40f, 0x68D987FF);
        sfxPlay(SFX_HEAL);
        return;
    }
    if (sourceMove && (strcmp(sourceMove->kind, "buff") == 0 ||
                        strcmp(sourceMove->kind, "potenciar") == 0 ||
                        strcmp(sourceMove->kind, "crecimiento") == 0)) {
        fighter->speedBoost = 5.0f;
        spawnEffect(FX_BURST, fighter->x, fighter->y, fighter->x, fighter->y,
                    0.36f, 0xE9C94BFF);
        sfxPlay(SFX_MOVE);
        return;
    }
    if (distance < 0.1f) return;
    if (sourceMove && (strcmp(sourceMove->kind, "mirror") == 0 ||
                       strcmp(sourceMove->kind, "escudo") == 0)) {
        fighter->dashIFrames = 0.85f;
        spawnEffect(FX_DODGE, fighter->x, fighter->y, fighter->x, fighter->y,
                    0.32f, 0xE986D1FF);
        sfxPlay(SFX_MOVE);
        return;
    }
    if (sourceMove && strcmp(sourceMove->kind, "blink") == 0) {
        float travel = fminf(100.0f, fmaxf(0.0f, distance - 40.0f));
        fighter->x = clampf(fighter->x + dx / distance * travel, 26.0f, 374.0f);
        fighter->y = clampf(fighter->y + dy / distance * travel, 96.0f, 184.0f);
        spawnEffect(FX_DODGE, startX, startY, fighter->x, fighter->y,
                    0.28f, 0xB48AEBFF);
        sfxPlay(SFX_DASH);
        return;
    }
    if (distance > move->range) return;
    sfxPlay(move->type == TYPE_FIRE ? SFX_FIRE : SFX_MOVE);
    dx /= distance;
    dy /= distance;
    playAttackAnimation(side);
    if (slot == 1 && move->style == MOVE_DASH)
        fighter->dashIFrames = fmaxf(fighter->dashIFrames, 0.25f);

    if (move->style == MOVE_MELEE) {
        if (distance > move->range) return;
        spawnEffect(FX_SLASH, startX + dx * 8.0f, startY + dy * 8.0f,
                    gFighter[opponent].x, gFighter[opponent].y, 0.22f, move->color);
        combatHit(side, move->damage, move->type, move->status, move->statusTime,
                  54.0f, 0.16f, move->color);
    } else if (move->style == MOVE_DASH) {
        float travel = fminf(move->range, fmaxf(0.0f, distance - 29.0f));
        fighter->x = clampf(fighter->x + dx * travel, 26.0f, 374.0f);
        fighter->y = clampf(fighter->y + dy * travel, 96.0f, 184.0f);
        spawnEffect(FX_SLASH, startX, startY, fighter->x, fighter->y,
                    0.24f, move->color);
        if (distance2(fighter->x, fighter->y, gFighter[opponent].x,
                      gFighter[opponent].y) <= 53.0f * 53.0f)
            combatHit(side, move->damage, move->type, move->status, move->statusTime,
                      70.0f, 0.22f, move->color);
    } else if (move->style == MOVE_BURST) {
        if (sourceMove && strcmp(sourceMove->kind, "chorro") == 0)
            spawnEffect(FX_BEAM, startX, startY, gFighter[opponent].x,
                        gFighter[opponent].y, 0.38f, move->color);
        else
            spawnEffect(FX_BURST, gFighter[opponent].x, gFighter[opponent].y,
                        gFighter[opponent].x, gFighter[opponent].y,
                        0.38f, move->color);
        combatHit(side, move->damage, move->type, move->status, move->statusTime,
                  42.0f, 0.14f, move->color);
    } else if (move->style == MOVE_SPREAD) {
        int pelletDamage = move->damage / 2;
        if (pelletDamage < 4) pelletDamage = 4;
        fireProjectile(side, move, dx, dy, pelletDamage, -0.16f);
        fireProjectile(side, move, dx, dy, pelletDamage, 0.0f);
        fireProjectile(side, move, dx, dy, pelletDamage, 0.16f);
    } else {
        spawnEffect(FX_SLASH, startX, startY, gFighter[opponent].x,
                    gFighter[opponent].y, 0.2f, move->color);
        fireProjectile(side, move, dx, dy, move->damage, 0.0f);
    }
}

static void updateShots(float dt)
{
    for (int i = 0; i < 8; ++i) {
        Shot *shot = &gShots[i];
        if (!shot->active) continue;
        shot->x += shot->vx * dt;
        shot->y += shot->vy * dt;
        shot->life -= dt;
        int target = shot->side == 0 ? 1 : 0;
        if (distance2(shot->x, shot->y, gFighter[target].x,
                      gFighter[target].y) < 23.0f * 23.0f) {
            combatHit(shot->side, shot->damage, (AttackType)shot->attackType,
                      (StatusKind)shot->status, shot->statusTime,
                      shot->knockback, 0.11f, shot->color);
            gFighter[shot->side].ultimate = clampf(gFighter[shot->side].ultimate + 4.0f,
                                                   0.0f, 100.0f);
            shot->active = 0;
        } else if (shot->life <= 0.0f || shot->x < 12.0f || shot->x > 388.0f ||
                   shot->y < 42.0f || shot->y > 230.0f) {
            shot->active = 0;
        }
    }
}

static void performBasicAttack(int side)
{
    playAttackAnimation(side);
    useMove(side, -1);
}

static void useBasic(int side)
{
    Combatant *fighter = &gFighter[side];
    if (fighter->basicCd > 0.0f || gScene != SCENE_BATTLE || fighter->hitStun > 0.0f)
        return;
    fighter->basicCd = moveFromBasic(&gTeam[side][gActive[side]]).cooldown;
    performBasicAttack(side);
}

static void useSkill(int slot)
{
    Combatant *fighter = &gFighter[0];
    Pokemon *activePokemon = &gTeam[0][gActive[0]];
    if (gScene != SCENE_BATTLE || slot < 0 || slot > 3 ||
        fighter->skillCd[slot] > 0.0f || fighter->hitStun > 0.0f)
        return;
    if (slot == 3 && fighter->ultimate < 100.0f) return;
    MoveDefinition move = moveFromRoster(activePokemon, slot);
    if (move.energy > fighter->energy) return;
    fighter->skillCd[slot] = move.cooldown;
    useMove(0, slot);
}

static void spawnSupply(void)
{
    static const float points[8][2] = {
        { 70, 104 }, { 200, 96 }, { 330, 104 }, { 72, 180 },
        { 200, 185 }, { 330, 180 }, { 120, 125 }, { 280, 125 }
    };
    int p = gSupplyCount % 8;
    gSupplyX = points[p][0];
    gSupplyY = points[p][1];
    gSupplyKind = gSupplyCount % 10 < 4 ? DROP_CHARGE : (gSupplyCount % 10 < 7 ? DROP_HEAL : DROP_SPEED);
    gSupplyLife = 12.0f;
    ++gSupplyCount;
}

static void updateSupply(float dt)
{
    if (gSupplyLife <= 0.0f) {
        gSupplyIn -= dt;
        if (gSupplyIn <= 0.0f) {
            spawnSupply();
            gSupplyIn = 18.0f;
        }
        return;
    }
    gSupplyLife -= dt;
    if (distance2(gFighter[0].x, gFighter[0].y, gSupplyX, gSupplyY) < 22.0f * 22.0f) {
        Pokemon *p = &gTeam[0][gActive[0]];
        if (gSupplyKind == DROP_HEAL)
            p->hp = p->hp + (int)(p->maxHp * 0.2f) > p->maxHp ? p->maxHp : p->hp + (int)(p->maxHp * 0.2f);
        else if (gSupplyKind == DROP_CHARGE)
            gFighter[0].ultimate = clampf(gFighter[0].ultimate + 20.0f, 0.0f, 100.0f);
        else
            gFighter[0].speedBoost = 6.0f;
        sfxPlay(gSupplyKind == DROP_HEAL ? SFX_HEAL : SFX_UI_CONFIRM);
        gSupplyLife = 0.0f;
    }
    if (gSupplyLife < 0.0f) gSupplyLife = 0.0f;
}

static void updateStatusEffects(int side, float dt)
{
    Combatant *fighter = &gFighter[side];
    if (fighter->burnTime > 0.0f) {
        fighter->burnTime = fmaxf(0.0f, fighter->burnTime - dt);
        fighter->burnTick -= dt;
        if (fighter->burnTick <= 0.0f && fighter->burnTime > 0.0f) {
            damagePokemon(side, 2, 0.0f);
            fighter->burnTick = 0.85f;
        }
    } else {
        fighter->burnTick = 0.0f;
    }
    fighter->slowTime = fmaxf(0.0f, fighter->slowTime - dt);
    fighter->hitStun = fmaxf(0.0f, fighter->hitStun - dt);
    fighter->comboTimer = fmaxf(0.0f, fighter->comboTimer - dt);
    if (fighter->comboTimer <= 0.0f) fighter->comboCount = 0;
}

static void updateEffects(float dt)
{
    for (int i = 0; i < 16; ++i)
        if (gEffects[i].life > 0.0f)
            gEffects[i].life = fmaxf(0.0f, gEffects[i].life - dt);
}

static void updateBattle(float dt, u32 held, u32 down, const circlePosition *pad)
{
    if (down & KEY_START) {
        gMenuSelection = 0;
        gScene = SCENE_PAUSE;
        return;
    }
    if (gGameMode != MODE_TRAINING)
        gBattleTime = fmaxf(0.0f, gBattleTime - dt);
    updateShots(dt);
    updateEffects(dt);
    Combatant *player = &gFighter[0];
    Combatant *cpu = &gFighter[1];
    updateStatusEffects(0, dt);
    updateStatusEffects(1, dt);
    for (int side = 0; side < 2; ++side) {
        Pokemon *pokemon = &gTeam[side][gActive[side]];
        Combatant *fighter = &gFighter[side];
        fighter->energy = clampf(fighter->energy + (100.0f / 3.0f) * dt, 0.0f, 100.0f);
        fighter->ultimate = clampf(fighter->ultimate + (100.0f / 3.0f) * dt, 0.0f, 100.0f);
        const Arena2dRosterEntry *line = &kArena2dRoster[pokemon->rosterIndex];
        if (pokemon->evolutionStage < line->evolutionCount) {
            float threshold = line->evolutionThreshold[pokemon->evolutionStage];
            pokemon->experience = fminf(threshold,
                pokemon->experience + threshold / 3.0f * dt);
        }
    }
    (void)transformPokemon(1);
    float moveX = (float)pad->dx / 156.0f;
    float moveY = -(float)pad->dy / 156.0f;
    /* The D-pad is a second movement option. It is only used when the Circle
       Pad is centred, so the two inputs cannot fight each other. */
    if (fabsf(moveX) < 0.18f && fabsf(moveY) < 0.18f) {
        if (held & KEY_LEFT) moveX = -1.0f;
        else if (held & KEY_RIGHT) moveX = 1.0f;
        if (held & KEY_UP) moveY = -1.0f;
        else if (held & KEY_DOWN) moveY = 1.0f;
    }
    float mag = sqrtf(moveX * moveX + moveY * moveY);
    if (mag > 1.0f) { moveX /= mag; moveY /= mag; }
    if (player->hitStun > 0.0f) moveX = moveY = 0.0f;
    player->moveX = moveX;
    player->moveY = moveY;
    float speciesSpeed = kArena2dForms[gTeam[0][0].formIndex].base[3];
    float speed = speciesSpeed * 0.44f;
    if (player->speedBoost > 0.0f) speed *= 1.25f;
    if (player->slowTime > 0.0f) speed *= 0.62f;
    player->x = clampf(player->x + (moveX * speed + player->knockbackX) * dt,
                       26.0f, 374.0f);
    player->y = clampf(player->y + (moveY * speed + player->knockbackY) * dt,
                       96.0f, 184.0f);
    player->knockbackX *= expf(-8.0f * dt);
    player->knockbackY *= expf(-8.0f * dt);

    float dx = player->x - cpu->x;
    float dy = player->y - cpu->y;
    float d = sqrtf(dx * dx + dy * dy);
    static const float cpuScale[3] = { 0.68f, 0.86f, 1.0f };
    static const float basicDelay[3] = { 1.10f, 0.90f, 0.67f };
    static const float skillDelay[3] = { 4.70f, 3.40f, 2.45f };
    MoveDefinition cpuBasicMove = moveFromBasic(&gTeam[1][0]);
    if (gGameMode != MODE_TRAINING && cpu->hitStun <= 0.0f &&
        d > cpuBasicMove.range * 0.72f && d > 0.1f) {
        float cpuMoveScale = cpu->slowTime > 0.0f ? 0.62f : 1.0f;
        float cpuSpeed = kArena2dForms[gTeam[1][0].formIndex].base[3] *
                         0.44f * cpuScale[gCpuDifficulty];
        cpu->x = clampf(cpu->x + (dx / d * cpuSpeed * cpuMoveScale +
                                  cpu->knockbackX) * dt, 26.0f, 374.0f);
        cpu->y = clampf(cpu->y + (dy / d * cpuSpeed * cpuMoveScale +
                                  cpu->knockbackY) * dt, 96.0f, 184.0f);
    } else if (gGameMode != MODE_TRAINING && cpu->hitStun <= 0.0f &&
               d <= cpuBasicMove.range) {
        gCpuBasicCd -= dt;
        if (gCpuBasicCd <= 0.0f) {
            gCpuBasicCd = basicDelay[gCpuDifficulty];
            performBasicAttack(1);
        }
    }
    cpu->knockbackX *= expf(-8.0f * dt);
    cpu->knockbackY *= expf(-8.0f * dt);

    if (gGameMode != MODE_TRAINING) gCpuSkillCd -= dt;
    if (gGameMode != MODE_TRAINING && cpu->hitStun <= 0.0f && gCpuSkillCd <= 0.0f) {
        int slot = (int)(gBattleTime / 3.0f) % 4;
        MoveDefinition cpuMove = moveFromRoster(&gTeam[1][gActive[1]], slot);
        if ((slot != 3 || cpu->ultimate >= 100.0f) &&
            (cpuMove.energy <= cpu->energy) &&
            (cpuMove.range <= 0.0f || d <= cpuMove.range))
            useMove(1, slot);
        gCpuSkillCd = skillDelay[gCpuDifficulty] + (float)slot * 0.45f;
    }

    player->attackAnim = fmaxf(0.0f, player->attackAnim - dt);
    cpu->attackAnim = fmaxf(0.0f, cpu->attackAnim - dt);
    player->animKind = player->attackAnim > 0.0f ? ANIM_SHOOT :
                       (mag > 0.15f ? ANIM_WALK : ANIM_IDLE);
    cpu->animKind = gGameMode == MODE_TRAINING ? ANIM_IDLE :
                    (cpu->attackAnim > 0.0f ? ANIM_SHOOT :
                     (d > 58.0f ? ANIM_WALK : ANIM_IDLE));
    player->animClock += dt;
    cpu->animClock += dt;
    player->basicCd = fmaxf(0.0f, player->basicCd - dt);
    cpu->basicCd = fmaxf(0.0f, cpu->basicCd - dt);
    player->switchCd = fmaxf(0.0f, player->switchCd - dt);
    player->dashIFrames = fmaxf(0.0f, player->dashIFrames - dt);
    player->speedBoost = fmaxf(0.0f, player->speedBoost - dt);
    player->hurtFlash = fmaxf(0.0f, player->hurtFlash - dt);
    cpu->dashIFrames = fmaxf(0.0f, cpu->dashIFrames - dt);
    cpu->hurtFlash = fmaxf(0.0f, cpu->hurtFlash - dt);
    cpu->switchCd = fmaxf(0.0f, cpu->switchCd - dt);
    for (int i = 0; i < 4; ++i)
        player->skillCd[i] = fmaxf(0.0f, player->skillCd[i] - dt);
    gHitFlash = fmaxf(0.0f, gHitFlash - dt);

    if ((held & KEY_A) || (down & KEY_A)) useBasic(0);
    if (down & KEY_B) useSkill(0);
    if (down & KEY_X) useSkill(1);
    if (down & KEY_Y) useSkill(2);
    if (down & KEY_R) useSkill(3);
    if (down & KEY_L) (void)transformPokemon(0);
    if (down & KEY_SELECT)
        switchPokemon(0, (gActive[0] + 1) % TEAM_SIZE);

    if (gGameMode != MODE_TRAINING) updateSupply(dt);
    if (livingCount(0) == 0 || livingCount(1) == 0 ||
        (gGameMode != MODE_TRAINING && gBattleTime <= 0.0f)) {
        if (livingCount(0) == 0) gWinner = 1;
        else if (livingCount(1) == 0) gWinner = 0;
        else {
            Pokemon *p = &gTeam[0][gActive[0]];
            Pokemon *e = &gTeam[1][gActive[1]];
            gWinner = (float)p->hp / p->maxHp >= (float)e->hp / e->maxHp ? 0 : 1;
        }
        ++gRoundWins[gWinner];
        gScene = SCENE_RESULT;
    }
}

static void drawArena(void)
{
    if (!loadArenaTexture(gArenaIndex)) return;
    C2D_DrawImageAt(gArenaImage, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
}

static void drawSegmentedBar(float x, float y, float w, float h, int hp, int maxHp)
{
    float ratio = maxHp > 0 ? clampf((float)hp / (float)maxHp, 0.0f, 1.0f) : 0.0f;
    rect(x, y, w, h, 0x11151DFF);
    const int segments = 16;
    float gap = 1.0f;
    float segmentWidth = (w - gap * (segments + 1)) / (float)segments;
    int filled = (int)ceilf(ratio * (float)segments);
    u32 color = ratio > 0.35f ? 0x52D15FFF : 0xF14E58FF;
    for (int i = 0; i < segments; ++i)
        rect(x + gap + i * (segmentWidth + gap), y + 1.0f,
             segmentWidth, h - 2.0f, i < filled ? color : 0x293344FF);
}

static void drawRoster(int side, float x, float y)
{
    for (int i = 0; i < TEAM_SIZE; ++i) {
        float bx = x + i * 24.0f;
        rect(bx, y, 21.0f, 19.0f, i == gActive[side] ? 0xF5C537FF : 0x172039FF);
        drawPokemonIcon(gTeam[side][i].rosterIndex, bx + 2.0f, y + 1.0f, 16.0f);
        if (gTeam[side][i].hp <= 0)
            rect(bx + 1.0f, y + 16.0f, 19.0f, 2.0f, C_RED);
    }
}

static void drawFighter(int side)
{
    Pokemon *pokemon = &gTeam[side][gActive[side]];
    Combatant *fighter = &gFighter[side];
    int opponent = side == 0 ? 1 : 0;
    float bob = fighter->animKind == ANIM_IDLE ? sinf(fighter->animClock * 4.5f) * 1.5f : 0.0f;
    int frameCount = p2dFrameCount[pokemon->speciesArt][fighter->animKind];
    float fps = fighter->animKind == ANIM_SHOOT ? 14.0f :
                (fighter->animKind == ANIM_WALK ? 10.0f : 5.0f);
    int frame = (int)(fighter->animClock * fps) % frameCount;
    int direction = faceDirection(gFighter[opponent].x - fighter->x,
                                  gFighter[opponent].y - fighter->y);
    float shadowW = p2dFrameHeight[pokemon->speciesArt][ANIM_IDLE] > 40 ? 30.0f : 22.0f;
    rect(fighter->x - shadowW * 0.5f, fighter->y + 2.0f,
         shadowW, 4.0f, 0x090D16AA);
    drawPokemonAt(side, pokemon->rosterIndex, pokemon->speciesArt,
                  pokemon->formIndex, fighter->animKind, direction, frame,
                  fighter->x, fighter->y + bob,
                  kArena2dForms[pokemon->formIndex].scale);
    if (fighter->hurtFlash > 0.0f)
        rect(fighter->x - 10.0f, fighter->y - 31.0f, 20.0f, 2.0f, 0xFFF3D9FF);
    if (side == 0 && fighter->dashIFrames > 0.0f)
        C2D_DrawCircleSolid(fighter->x, fighter->y - 6.0f, 0.25f, 23.0f,
                            c2dColor(0xE9F8FF55));
    if (fighter->burnTime > 0.0f)
        centeredText(fighter->x, fighter->y - 49.0f, 0.19f, 0xFF875AFF, "BURN");
    else if (fighter->slowTime > 0.0f)
        centeredText(fighter->x, fighter->y - 49.0f, 0.19f, 0x7CE1F2FF, "SLOW");
    else if (fighter->hitStun > 0.0f)
        centeredText(fighter->x, fighter->y - 49.0f, 0.19f, 0xFFE678FF, "STUN");
}

static void drawAttackEffects(void)
{
    for (int i = 0; i < 16; ++i) {
        AttackEffect *effect = &gEffects[i];
        if (effect->life <= 0.0f || effect->maxLife <= 0.0f) continue;
        float progress = 1.0f - effect->life / effect->maxLife;
        if (effect->kind == FX_BURST) {
            float radius = 6.0f + progress * 27.0f;
            for (int spoke = 0; spoke < 8; ++spoke) {
                float angle = (float)spoke * 0.78539816339f;
                float inner = radius * 0.55f;
                float x0 = effect->x + cosf(angle) * inner;
                float y0 = effect->y + sinf(angle) * inner;
                float x1 = effect->x + cosf(angle) * radius;
                float y1 = effect->y + sinf(angle) * radius;
                C2D_DrawLine(x0, y0, c2dColor(effect->color), x1, y1,
                             c2dColor(effect->color), 3.0f, 0.29f);
            }
        } else {
            float thickness = effect->kind == FX_DODGE ? 7.0f : 5.0f;
            C2D_DrawLine(effect->x, effect->y, c2dColor(effect->color),
                         effect->endX, effect->endY, c2dColor(effect->color),
                         thickness, 0.29f);
            if (effect->kind == FX_SLASH) {
                float dx = effect->endX - effect->x;
                float dy = effect->endY - effect->y;
                float length = sqrtf(dx * dx + dy * dy);
                if (length > 1.0f) {
                    float ox = -dy / length * 4.0f;
                    float oy = dx / length * 4.0f;
                    C2D_DrawLine(effect->x + ox, effect->y + oy,
                                 c2dColor(effect->color), effect->endX + ox,
                                 effect->endY + oy, c2dColor(effect->color),
                                 2.0f, 0.28f);
                }
            }
        }
    }
}

static void drawBattleTop(void)
{
    drawArena();
    Pokemon *p = &gTeam[0][gActive[0]];
    Pokemon *e = &gTeam[1][gActive[1]];
    /* Two compact cards reproduce the source HUD rather than the prior
       console-game panels.  The cards float above the arena crowd. */
    rect(5, 4, 153, 39, 0x111A29F5);
    rect(5, 4, 153, 2, 0x00E676FF);
    rect(8, 9, 29, 29, 0x0A101AFF);
    drawPokemonIcon(p->rosterIndex, 9.0f, 10.0f, 25.0f);
    fittedText(42, 8, 108.0f, 0.31f, 0xFFFFFFFF, p->name, false);
    fittedText(42, 19, 108.0f, 0.19f, 0xB7C7DCFF, p->type, false);
    drawSegmentedBar(42, 30, 108, 7, p->hp, p->maxHp);
    rect(242, 4, 153, 39, 0x111A29F5);
    rect(242, 4, 153, 2, 0xFF1744FF);
    rect(362, 9, 29, 29, 0x0A101AFF);
    drawPokemonIcon(e->rosterIndex, 363.0f, 10.0f, 25.0f);
    fittedText(250, 8, 107.0f, 0.31f, 0xFFFFFFFF, e->name, false);
    fittedText(250, 19, 107.0f, 0.19f, 0xB7C7DCFF, e->type, false);
    drawSegmentedBar(250, 30, 107, 7, e->hp, e->maxHp);
    char clockText[16];
    int seconds = (int)gBattleTime;
    if (gGameMode == MODE_TRAINING)
        snprintf(clockText, sizeof(clockText), "TRAIN");
    else
        snprintf(clockText, sizeof(clockText), "%02d:%02d", seconds / 60, seconds % 60);
    rect(165, 4, 70, 34, 0x0B101BF5);
    centeredText(200, 8, 0.37f, 0xFFFFFFFF, clockText);
    char roundText[16];
    snprintf(roundText, sizeof(roundText), "ROUND %d", gRoundNumber);
    centeredText(200, 24, 0.18f, 0xFFD740FF, roundText);

    if (gSupplyLife > 0.0f) {
        u32 supplyColor = gSupplyKind == DROP_HEAL ? 0xF48CC8FF :
                          (gSupplyKind == DROP_CHARGE ? 0xB884F4FF : C_GOLD);
        C2D_DrawCircleSolid(gSupplyX, gSupplyY, 0.35f, 9.0f,
                            c2dColor(0x18243AFF));
        C2D_DrawCircleSolid(gSupplyX, gSupplyY, 0.30f, 6.0f,
                            c2dColor(supplyColor));
    }
    for (int i = 0; i < 8; ++i) {
        Shot *shot = &gShots[i];
        if (!shot->active) continue;
        float trailX = shot->x - shot->vx * 0.035f;
        float trailY = shot->y - shot->vy * 0.035f;
        C2D_DrawLine(trailX, trailY, c2dColor(shot->color), shot->x, shot->y,
                     c2dColor(shot->color), 6.0f, 0.37f);
        C2D_DrawCircleSolid(shot->x, shot->y, 0.36f, 4.8f,
                            c2dColor(shot->color));
    }
    drawAttackEffects();
    drawFighter(0);
    drawFighter(1);
    if (gHitFlash > 0.0f)
        centeredText(200, 52, 0.28f, C_GOLD, gHitLabel);
}

static void webActionButton(float x, const char *key, const char *label,
                            float cooldown, float energyCost, u32 tint)
{
    const float y = 85.0f, w = 59.0f, h = 77.0f;
    rect(x, y, w, h, 0x1A2434FF);
    rect(x + 2, y + 2, w - 4, h - 4,
         cooldown > 0.0f || energyCost > gFighter[0].energy ? 0x263448FF : 0x315273FF);
    rect(x + 2, y + 2, 3, h - 4, tint);
    rect(x + 8, y + 7, 19, 16, 0x111A29FF);
    centeredText(x + 17.5f, y + 8, 0.26f, 0xFFD740FF, key);
    fittedText(x + 8, y + 29, w - 12, 0.22f, 0xFFFFFFFF, label, true);
    if (energyCost > 0.0f) {
        char cost[12];
        snprintf(cost, sizeof(cost), "E %.0f", energyCost);
        fittedText(x + 8, y + 45, w - 12, 0.17f, 0xB7C7DCFF, cost, true);
    }
    if (cooldown > 0.0f) {
        char cd[12];
        snprintf(cd, sizeof(cd), "%.1f s", cooldown);
        fittedText(x + 8, y + 59, w - 12, 0.16f, 0xFFD740FF, cd, true);
    }
}

static void drawBottomBattle(void)
{
    Pokemon *activePokemon = &gTeam[0][gActive[0]];
    rect(0, 0, BOTTOM_W, BOTTOM_H, 0x090E18FF);
    rect(0, 0, BOTTOM_W, 3, 0xE8B940FF);
    drawPokemonIcon(activePokemon->rosterIndex, 8, 7, 28);
    fittedText(42, 8, 263, 0.31f, 0xFFFFFFFF, activePokemon->name, false);
    fittedText(42, 22, 263, 0.18f, 0xAFC3D8FF, activePokemon->type, false);
    drawSegmentedBar(42, 36, 263, 8, activePokemon->hp, activePokemon->maxHp);
    fittedText(8, 50, 70, 0.19f, 0xAFC3D8FF, "ENERGY", false);
    rect(66, 51, 245, 6, 0x1C2636FF);
    rect(67, 52, 243.0f * gFighter[0].energy / 100.0f, 4, 0x54BCEBFF);
    fittedText(8, 62, 70, 0.19f, 0xAFC3D8FF, "ULTIMATE", false);
    rect(66, 63, 245, 6, 0x1C2636FF);
    rect(67, 64, 243.0f * gFighter[0].ultimate / 100.0f, 4, 0xF1BE48FF);
    const Arena2dRosterEntry *line = &kArena2dRoster[activePokemon->rosterIndex];
    if (activePokemon->evolutionStage < line->evolutionCount) {
        float threshold = line->evolutionThreshold[activePokemon->evolutionStage];
        float progress = threshold > 0.0f ?
            clampf(activePokemon->experience / threshold, 0.0f, 1.0f) : 0.0f;
        fittedText(8, 72, 52, 0.17f, 0xAFC3D8FF, "EVOLVE", false);
        rect(66, 73, 245, 5, 0x1C2636FF);
        rect(67, 74, 243.0f * progress, 3, 0x7EDB88FF);
    } else {
        fittedText(8, 72, 70, 0.17f, 0xAFC3D8FF,
                   activePokemon->isMega ? "MEGA" : "MAX LEVEL", false);
    }
    const Arena2dMoveEntry *basic = rosterBasicMove(activePokemon);
    const Arena2dMoveEntry *b = rosterMove(activePokemon, 0);
    const Arena2dMoveEntry *x = rosterMove(activePokemon, 1);
    const Arena2dMoveEntry *y = rosterMove(activePokemon, 2);
    const Arena2dMoveEntry *r = rosterMove(activePokemon, 3);
    webActionButton(6, "A", basic ? basic->name : "ATTACK", gFighter[0].basicCd, 0, 0x68C5F0FF);
    webActionButton(69, "B", b ? b->name : "SKILL", gFighter[0].skillCd[0], b ? b->energy : 0, 0xEA7654FF);
    webActionButton(132, "X", x ? x->name : "DODGE", gFighter[0].skillCd[1], x ? x->energy : 0, 0x63D7C5FF);
    webActionButton(195, "Y", y ? y->name : "SKILL", gFighter[0].skillCd[2], y ? y->energy : 0, 0x85C76DFF);
    webActionButton(258, "R", r ? r->name : "ULTIMATE", gFighter[0].skillCd[3], r ? r->energy : 0, 0xE8B940FF);
    fittedText(8, 170, 304, 0.19f, 0xC2CBD8FF,
               "CIRCLE PAD MOVE    L EVOLVE/MEGA   START PAUSE", false);
    fittedText(8, 183, 304, 0.18f, 0x8C9CB2FF,
               "A BASIC   B RMB   X SPACE   Y Q   R E", false);
    const char *modeLabel = gGameMode == MODE_TRAINING ? "TRAINING" :
                            (gGameMode == MODE_STORY ? "LEAGUE" :
                             (gGameMode == MODE_RUN ? "ARENA CHALLENGE" : "VS CPU"));
    char footer[48];
    snprintf(footer, sizeof(footer), "POKE ARENA 2D   /   %s", modeLabel);
    fittedText(8, 210, 304, 0.20f, 0xE8B940FF, footer, false);
}

static void drawMenuBackdrop(void)
{
    rect(0, 0, BOTTOM_W, BOTTOM_H, 0x183F70FF);
    for (int y = 0; y < 240; y += 16) {
        for (int x = 0; x < 320; x += 16) {
            if (((x / 16) + (y / 16)) % 2 == 0)
                rect((float)x, (float)y, 16.0f, 16.0f, 0x1B477AFF);
        }
    }
    rect(0, 0, BOTTOM_W, 4, 0xF6C64AFF);
}

static void drawMenuRow(float y, const char *title, const char *subtitle,
                        const char *status, bool selected, bool disabled)
{
    u32 fill = selected ? 0xF5C54DFF : 0x214D82FF;
    u32 edge = selected ? 0xFFF0A8FF : 0x78AEE2FF;
    u32 mainColor = selected ? 0x102849FF : (disabled ? 0x879CBAFF : C_TEXT);
    u32 subColor = selected ? 0x5D4A20FF : 0xB7D1EDFF;
    rect(7, y, 306, 23, 0x0B2D55FF);
    rect(8, y, 304, 21, fill);
    rect(8, y, 304, 1, edge);
    rect(8, y, 2, 21, edge);
    if (selected) rect(8, y, 4, 21, 0xFFFFFFFF);
    fittedText(17, y + 1.0f, status ? 207.0f : 280.0f, 0.31f,
               mainColor, title, false);
    if (subtitle && subtitle[0])
        fittedText(17, y + 14.0f, status ? 207.0f : 280.0f, 0.18f,
                   subColor, subtitle, false);
    if (status)
        fittedText(231, y + 5, 74.0f, 0.28f,
                   selected ? 0x273047FF : C_GOLD, status, true);
}

static void drawMenuHeader(const char *title)
{
    drawMenuBackdrop();
    fittedText(160, 6, 300, 0.48f, C_GOLD, title, true);
}

static void drawHomeMenuBottom(void)
{
    static const char *const titles[] = {
        "PLAY", "ONLINE", "ROSTER", "POKEDEX", "TUTORIAL", "SETTINGS", "CREDITS"
    };
    static const char *const subtitles[] = {
        "CHOOSE A GAME MODE", "ONLINE BATTLES", "CHOOSE YOUR FIGHTER", "VIEW YOUR POKEMON",
        "CONTROLS AND RULES", "ARENA AND TEXT SIZE", "PROJECT CREDITS"
    };
    drawMenuHeader("POKE ARENA 2D");
    for (int i = 0; i < 7; ++i) {
        drawMenuRow(27.0f + (float)i * 25.0f, titles[i], subtitles[i],
                    NULL, gMenuSelection == i, false);
        drawWebUiIcon(i, 17.0f, 30.0f + (float)i * 25.0f, 16.0f);
    }
    textAt(10, 210, 0.33f, C_GOLD, "A / TOUCH: SELECT");
    textAt(10, 224, 0.27f, C_MUTED, "B: BACK");
}

static void drawPlayMenuBottom(void)
{
    static const char *const titles[] = {
        "LEAGUE", "VS CPU", "2 PLAYERS", "TRAINING", "CHALLENGE", "BACK"
    };
    static const char *const subtitles[] = {
        "PLAY A SERIES OF DUELS", "PICK YOUR CPU DIFFICULTY",
        "LOCAL TWO PLAYER IS NOT READY", "QUICK PRACTICE BATTLE",
        "SURVIVE SIX FIGHTS", "RETURN TO MAIN MENU"
    };
    drawMenuHeader("PLAY");
    for (int i = 0; i < 6; ++i)
        drawMenuRow(30.0f + (float)i * 27.0f, titles[i], subtitles[i],
                    i == 2 ? "SOON" : NULL, gMenuSelection == i, i == 2);
    fittedText(10, 211, 300.0f, 0.32f, C_GOLD, "A / TOUCH: SELECT  B: BACK", false);
}

static void drawTitle(void)
{
    C2D_DrawImageAt(gWebTitle, 0, 0, 0, NULL, 1.0f, 1.0f);
    C2D_DrawImageAt(gGameLogo, 53.0f, 7.0f, 0.10f, NULL, 1.0f, 1.0f);
    int playerForm = kArena2dRoster[gPlayerSpecies[0]].baseForm;
    int opponentForm = kArena2dRoster[gOpponentSpecies].baseForm;
    drawPokemonAt(0, gPlayerSpecies[0], kArena2dForms[playerForm].spriteArt,
                  playerForm, ANIM_IDLE, 2, 0,
                  88.0f, 218.0f, kArena2dForms[playerForm].scale);
    drawPokemonAt(1, gOpponentSpecies, kArena2dForms[opponentForm].spriteArt,
                  opponentForm, ANIM_IDLE, 6, 0,
                  312.0f, 216.0f, kArena2dForms[opponentForm].scale);
}

static void drawTitleBottom(void)
{
    if (gMenuPage == PAGE_HOME) {
        drawHomeMenuBottom();
        return;
    }
    if (gMenuPage == PAGE_PLAY) {
        drawPlayMenuBottom();
        return;
    }

    switch (gMenuPage) {
    case PAGE_CPU: {
        static const char *const diff[] = { "EASY", "NORMAL", "HARD" };
        drawMenuHeader("VS CPU");
        char player[48], opponent[48];
        snprintf(player, sizeof(player), "PLAYER   %s", kArena2dRoster[gPlayerSpecies[0]].name);
        snprintf(opponent, sizeof(opponent), "CPU      %s", kArena2dRoster[gOpponentSpecies].name);
        drawMenuRow(31, player, "LEFT / RIGHT: CHOOSE POKEMON", NULL, gMenuSelection == 0, false);
        drawMenuRow(58, opponent, "LEFT / RIGHT: CHOOSE OPPONENT", NULL, gMenuSelection == 1, false);
        drawMenuRow(85, diff[gCpuDifficulty], "CPU DIFFICULTY", NULL, gMenuSelection == 2, false);
        drawMenuRow(120, "START BATTLE", "A / START", "A", gMenuSelection == 3, false);
        textAt(10, 211, 0.32f, C_GOLD, "A: SELECT  /  B: BACK");
        break;
    }
    case PAGE_STORY: {
        int first = (gStoryStage / 6) * 6;
        drawMenuHeader("CHOOSE A LEAGUE DUEL");
        for (int row = 0; row < 6; ++row) {
            int stage = first + row;
            if (stage >= 16) break;
            char title[24], status[16];
            snprintf(title, sizeof(title), "DUEL %02d", stage + 1);
            snprintf(status, sizeof(status), "%s", stage <= gStoryUnlocked ?
                     (stage < gStoryUnlocked ? "CLEARED" : "READY") : "LOCKED");
            drawMenuRow(29.0f + (float)row * 27.0f, title,
                        "SINGLE POKEMON MATCH", status, stage == gStoryStage,
                        stage > gStoryUnlocked);
        }
        fittedText(10, 196, 300.0f, 0.28f, C_MUTED, "LEFT / RIGHT: CHANGE PAGE", false);
        textAt(10, 216, 0.31f, C_GOLD, "A: PLAY  /  B: BACK");
        break;
    }
    case PAGE_RUN:
        drawMenuHeader("ARENA CHALLENGE");
        drawMenuRow(40, "START RUN", "SIX FIGHTS, ONE TEAM", "A", true, false);
        drawMenuRow(72, "TEAM CARRYOVER", "CURRENT HP CONTINUES", "", false, false);
        fittedText(10, 120, 300.0f, 0.32f, C_MUTED, "WIN EACH FIGHT TO REACH THE NEXT", false);
        textAt(10, 211, 0.32f, C_GOLD, "A: START  /  B: BACK");
        break;
    case PAGE_TEAMS: {
        drawMenuHeader("SELECT YOUR FIGHTER");
        for (int i = 0; i < ARENA2D_ROSTER_COUNT; ++i) {
            int row = i / 7, col = i % 7;
            float x = 2.0f + (float)col * 45.0f;
            float y = 30.0f + (float)row * 34.0f;
            const Arena2dRosterEntry *entry = &kArena2dRoster[i];
            bool focused = i == gMenuSelection;
            bool chosen = i == gPlayerSpecies[0];
            rect(x, y, 43.0f, 32.0f,
                 focused ? 0xF5C54DFF : (chosen ? 0x345F8CFF : 0x21416CFF));
            drawPokemonIcon(i, x + 12.0f, y + 1.0f, 19.0f);
            fittedText(x + 1.0f, y + 21.0f, 41.0f, 0.17f,
                       focused ? 0x102849FF : C_TEXT, entry->name, true);
        }
        fittedText(8, 205, 304.0f, 0.26f, C_MUTED,
                   "33 FIGHTERS  D-PAD: BROWSE  A: CHOOSE", false);
        fittedText(8, 222, 304.0f, 0.29f, C_GOLD,
                   "START: BATTLE  /  B: BACK", false);
        break;
    }
    case PAGE_ONLINE:
        drawMenuHeader("ONLINE PLAY");
        drawMenuRow(39, "RANKED", "CLIMB THE GLOBAL LADDER", "", false, true);
        drawMenuRow(67, "CREATE ROOM", "PLAY WITH A FRIEND", "", false, true);
        drawMenuRow(95, "FIND ROOM", "JOIN AN ONLINE MATCH", "", false, true);
        rect(8, 136, 304, 48, 0x14375FFF);
        fittedText(160, 141, 284, 0.34f, C_GOLD,
                   "NETWORK PLAY IS NOT AVAILABLE", true);
        fittedText(160, 158, 284, 0.29f, C_TEXT,
                   "IN THIS 3DS VERSION", true);
        textAt(10, 211, 0.32f, C_GOLD, "B: BACK");
        break;
    case PAGE_SETTINGS: {
        drawMenuHeader("SETTINGS");
        char arena[32], cpu[32], size[32];
        snprintf(arena, sizeof(arena), "ARENA: %s", kArenaNames[gArenaIndex]);
        snprintf(cpu, sizeof(cpu), "CPU: %s", gCpuDifficulty == 0 ? "EASY" :
                 (gCpuDifficulty == 1 ? "NORMAL" : "HARD"));
        snprintf(size, sizeof(size), "TEXT SIZE: %s", gUiTextScale > 1.05f ? "LARGE" : "NORMAL");
        drawMenuRow(35, arena, "CHANGE BATTLE BACKGROUND", "LEFT/RIGHT", gMenuSelection == 0, false);
        drawMenuRow(64, cpu, "DEFAULT CPU LEVEL", "LEFT/RIGHT", gMenuSelection == 1, false);
        drawMenuRow(93, size, "MAKE LABELS LARGER", "LEFT/RIGHT", gMenuSelection == 2, false);
        rect(8, 132, 304, 54, 0x14375FFF);
        fittedText(160, 138, 284, 0.33f, C_GOLD, "CONTROLS", true);
        fittedText(160, 157, 284, 0.27f, C_TEXT, "PAD/D-PAD MOVE   A BASIC   B/X/Y/R MOVES", true);
        fittedText(160, 172, 284, 0.27f, C_TEXT, "X SPACE MOVE   L EVOLVE/MEGA   START PAUSE", true);
        fittedText(10, 211, 300.0f, 0.29f, C_GOLD, "A / LEFT / RIGHT: CHANGE  B: BACK", false);
        break;
    }
    case PAGE_DEX:
        drawMenuHeader("POKEDEX");
        {
            int first = (gMenuSelection / 6) * 6;
            for (int row = 0; row < 6 && first + row < ARENA2D_ROSTER_COUNT; ++row) {
                int i = first + row;
                const Arena2dRosterEntry *entry = &kArena2dRoster[i];
                float y = 29.0f + row * 27.0f;
                bool selected = i == gMenuSelection;
                drawMenuRow(y, entry->name,
                            kArena2dForms[entry->baseForm].type1, NULL, selected, false);
                drawPokemonIcon(i, 284.0f, y + 1.0f, 19.0f);
            }
        }
        fittedText(10, 211, 300.0f, 0.32f, C_GOLD, "33 POKEMON  UP/DOWN: BROWSE  B: BACK", false);
        break;
    case PAGE_TUTORIAL:
        drawMenuHeader("HOW TO PLAY");
        drawMenuRow(34, "MOVE", "CIRCLE PAD OR D-PAD MOVES YOU", NULL, false, false);
        drawMenuRow(62, "ATTACK", "A CHAINS THREE BASIC HITS", NULL, false, false);
        drawMenuRow(90, "SPECIAL MOVES", "B / X / Y / R USE YOUR POKEMON'S KIT", NULL, false, false);
        drawMenuRow(118, "DODGE", "X USES YOUR POKEMON'S SPACE MOVE", NULL, false, false);
        drawMenuRow(146, "EVOLVE", "L WHEN YOUR EVOLUTION METER IS READY", NULL, false, false);
        drawMenuRow(174, "ENERGY AND STATUS", "SPECIALS COST ENERGY; SOME BURN OR STUN",
                    NULL, false, false);
        textAt(10, 211, 0.32f, C_GOLD, "B: BACK");
        break;
    case PAGE_CREDITS:
        drawMenuHeader("CREDITS");
        drawMenuRow(36, "INSPIRED BY", "POKE ARENA 2D", NULL, false, false);
        drawMenuRow(64, "POKEMON SPRITES", "PMDCOLLAB COMMUNITY", NULL, false, false);
        drawMenuRow(92, "UI FONT", "OPEN SANS REGULAR - APACHE 2.0", NULL, false, false);
        drawMenuRow(120, "3DS HOMEBREW", "BUILT FOR OLD 2DS AND 3DS", NULL, false, false);
        textAt(10, 211, 0.32f, C_GOLD, "B: BACK");
        break;
    case PAGE_2PLAY:
        drawMenuHeader("2 PLAYERS");
        rect(8, 45, 304, 94, 0x14375FFF);
        fittedText(160, 57, 280, 0.38f, C_GOLD,
                   "LOCAL TWO PLAYER IS NOT READY", true);
        fittedText(160, 82, 280, 0.32f, C_TEXT,
                   "THIS BUILD SUPPORTS OFFLINE CPU", true);
        fittedText(160, 101, 280, 0.32f, C_TEXT,
                   "BATTLES AND TRAINING", true);
        textAt(10, 211, 0.32f, C_GOLD, "B: BACK");
        break;
    case PAGE_HOME:
    case PAGE_PLAY:
        drawHomeMenuBottom();
        break;
    }
}

static void drawResult(void)
{
    bool matchComplete = gRoundWins[0] >= 2 || gRoundWins[1] >= 2;
    drawArena();
    rect(48, 70, 304, 94, 0x1C2743EE);
    rect(48, 70, 304, 3, gWinner == 0 ? C_GREEN : C_RED);
    fittedText(200, 82, 286, 0.94f, gWinner == 0 ? C_GREEN : C_RED,
               matchComplete ? (gWinner == 0 ? "VICTORY" : "DEFEAT") :
                               (gWinner == 0 ? "ROUND WON" : "ROUND LOST"), true);
    fittedText(200, 119, 280, 0.46f, C_TEXT, "A: CONTINUE   B: MAIN MENU", true);
}

static void drawResultBottom(void)
{
    rect(0, 0, BOTTOM_W, BOTTOM_H, C_BG);
    char score[32];
    snprintf(score, sizeof(score), "ROUND %d   P1 %d - %d CPU", gRoundNumber,
             gRoundWins[0], gRoundWins[1]);
    textAt(12, 18, 0.48f, C_TEXT, score);
    if (gGameMode == MODE_STORY) {
        char stage[32];
        snprintf(stage, sizeof(stage), "CHAPTER %02d %s", gStoryStage + 1,
                 gWinner == 0 ? "CLEAR" : "FAILED");
        textAt(12, 56, 0.42f, C_MUTED, stage);
    } else if (gGameMode == MODE_RUN) {
        char floor[32];
        snprintf(floor, sizeof(floor), "FLOOR %d OF 6 %s", gRunFloor,
                 gWinner == 0 ? "CLEAR" : "FAILED");
        textAt(12, 56, 0.42f, C_MUTED, floor);
    } else {
        fittedText(12, 56, 296.0f, 0.42f, C_MUTED, "A STARTS ANOTHER BATTLE", false);
    }
    fittedText(12, 82, 296.0f, 0.42f, C_MUTED, "B RETURNS TO THE MAIN MENU", false);
}

static void setMenuPage(MenuPage page)
{
    gMenuPage = page;
    gMenuSelection = 0;
    if (page == PAGE_TEAMS) gMenuSelection = gPlayerSpecies[0];
    if (page == PAGE_HOME) gMenuDepth = 0;
}

static void pushMenuPage(MenuPage page)
{
    if (gMenuDepth < 4) gMenuStack[gMenuDepth++] = gMenuPage;
    setMenuPage(page);
}

static void goBackMenu(void)
{
    sfxPlay(SFX_UI_BACK);
    MenuPage page = gMenuDepth > 0 ? gMenuStack[--gMenuDepth] : PAGE_HOME;
    setMenuPage(page);
}

static void startMode(GameMode mode)
{
    gGameMode = mode;
    gRoundWins[0] = gRoundWins[1] = 0;
    gRoundNumber = 1;
    if (mode == MODE_RUN) gRunFloor = 1;
    resetBattle();
}

static void activateHomeSelection(void)
{
    sfxPlay(SFX_UI_SELECT);
    switch (gMenuSelection) {
    case 0: pushMenuPage(PAGE_PLAY); break;
    case 1: pushMenuPage(PAGE_ONLINE); break;
    case 2: pushMenuPage(PAGE_TEAMS); break;
    case 3: pushMenuPage(PAGE_DEX); break;
    case 4: pushMenuPage(PAGE_TUTORIAL); break;
    case 5: pushMenuPage(PAGE_SETTINGS); break;
    case 6: pushMenuPage(PAGE_CREDITS); break;
    default: setMenuPage(PAGE_HOME); break;
    }
}

static void activatePlaySelection(void)
{
    sfxPlay(SFX_UI_SELECT);
    switch (gMenuSelection) {
    case 0: pushMenuPage(PAGE_STORY); gStoryStage = gStoryUnlocked; break;
    case 1: pushMenuPage(PAGE_CPU); break;
    case 2: pushMenuPage(PAGE_2PLAY); break;
    case 3: startMode(MODE_TRAINING); break;
    case 4: pushMenuPage(PAGE_RUN); gRunFloor = 1; break;
    case 5: goBackMenu(); break;
    default: break;
    }
}

static void selectPlayerSpecies(int slot, int species)
{
    if (slot < 0 || slot >= TEAM_SIZE) return;
    int candidate = (species % ARENA2D_ROSTER_COUNT + ARENA2D_ROSTER_COUNT) %
                    ARENA2D_ROSTER_COUNT;
    for (int tries = 0; tries < ARENA2D_ROSTER_COUNT; ++tries) {
        bool used = false;
        for (int i = 0; i < TEAM_SIZE; ++i)
            if (i != slot && gPlayerSpecies[i] == candidate) used = true;
        if (!used) {
            gPlayerSpecies[slot] = candidate;
            return;
        }
        candidate = (candidate + 1) % ARENA2D_ROSTER_COUNT;
    }
}

static void adjustSetting(int delta)
{
    if (gMenuSelection == 0) {
        gArenaIndex = (gArenaIndex + delta + ARENA_COUNT) % ARENA_COUNT;
        (void)loadArenaTexture(gArenaIndex);
    } else if (gMenuSelection == 1)
        gCpuDifficulty = (gCpuDifficulty + delta + 3) % 3;
    else if (gMenuSelection == 2)
        gUiTextScale = gUiTextScale > 1.05f ? 1.0f : 1.15f;
}

/* Menus accept both the D-pad and Circle Pad. A positive Circle Pad Y value
   means physical up on a 3DS; screen lists therefore map negative Y to down. */
static u32 menuInputFromStick(u32 down, const circlePosition *pad, float dt)
{
    u32 input = down;
    if (down & KEY_CPAD_UP) input |= KEY_UP;
    if (down & KEY_CPAD_DOWN) input |= KEY_DOWN;
    if (down & KEY_CPAD_LEFT) input |= KEY_LEFT;
    if (down & KEY_CPAD_RIGHT) input |= KEY_RIGHT;

    int absX = pad->dx < 0 ? -pad->dx : pad->dx;
    int absY = pad->dy < 0 ? -pad->dy : pad->dy;
    u32 direction = 0;
    if (absX >= 35 || absY >= 35) {
        if (absX > absY) direction = pad->dx < 0 ? KEY_LEFT : KEY_RIGHT;
        else direction = pad->dy < 0 ? KEY_DOWN : KEY_UP;
    }

    if (!direction) {
        gMenuStickDirection = 0;
        gMenuStickRepeat = 0.0f;
    } else if (direction != gMenuStickDirection) {
        gMenuStickDirection = direction;
        gMenuStickRepeat = 0.30f;
        input |= direction;
    } else {
        gMenuStickRepeat -= dt;
        if (gMenuStickRepeat <= 0.0f) {
            gMenuStickRepeat = 0.12f;
            input |= direction;
        }
    }
    return input;
}

static void handleMenuInput(u32 down)
{
    if (gScene != SCENE_TITLE) return;
    if (gMenuPage == PAGE_HOME) {
        if (down & KEY_UP) gMenuSelection = (gMenuSelection + 6) % 7;
        if (down & KEY_DOWN) gMenuSelection = (gMenuSelection + 1) % 7;
        if (down & KEY_START) { pushMenuPage(PAGE_PLAY); return; }
        if (down & KEY_A) activateHomeSelection();
        return;
    }

    if (gMenuPage == PAGE_PLAY) {
        if (down & KEY_UP) gMenuSelection = (gMenuSelection + 5) % 6;
        if (down & KEY_DOWN) gMenuSelection = (gMenuSelection + 1) % 6;
        if (down & (KEY_A | KEY_START)) activatePlaySelection();
        if (down & KEY_B) goBackMenu();
        return;
    }

    if (down & KEY_B) { goBackMenu(); return; }
    switch (gMenuPage) {
    case PAGE_CPU:
        if (down & KEY_UP) gMenuSelection = (gMenuSelection + 3) % 4;
        if (down & KEY_DOWN) gMenuSelection = (gMenuSelection + 1) % 4;
        if (gMenuSelection == 0 && (down & KEY_LEFT))
            gPlayerSpecies[0] = (gPlayerSpecies[0] + ARENA2D_ROSTER_COUNT - 1) % ARENA2D_ROSTER_COUNT;
        if (gMenuSelection == 0 && (down & KEY_RIGHT))
            gPlayerSpecies[0] = (gPlayerSpecies[0] + 1) % ARENA2D_ROSTER_COUNT;
        if (gMenuSelection == 1 && (down & KEY_LEFT))
            gOpponentSpecies = (gOpponentSpecies + ARENA2D_ROSTER_COUNT - 1) % ARENA2D_ROSTER_COUNT;
        if (gMenuSelection == 1 && (down & KEY_RIGHT))
            gOpponentSpecies = (gOpponentSpecies + 1) % ARENA2D_ROSTER_COUNT;
        if (gMenuSelection == 2 && (down & KEY_LEFT))
            gCpuDifficulty = (gCpuDifficulty + 2) % 3;
        if (gMenuSelection == 2 && (down & KEY_RIGHT))
            gCpuDifficulty = (gCpuDifficulty + 1) % 3;
        if ((down & (KEY_A | KEY_START)) && gMenuSelection == 3)
            startMode(MODE_CPU);
        break;
    case PAGE_STORY:
        if (down & KEY_UP) gStoryStage = (gStoryStage + 15) % 16;
        if (down & KEY_DOWN) gStoryStage = (gStoryStage + 1) % 16;
        if (down & KEY_LEFT) gStoryStage = (gStoryStage + 10) % 16;
        if (down & KEY_RIGHT) gStoryStage = (gStoryStage + 6) % 16;
        if ((down & (KEY_A | KEY_START)) && gStoryStage <= gStoryUnlocked)
            startMode(MODE_STORY);
        break;
    case PAGE_RUN:
        if (down & (KEY_A | KEY_START)) startMode(MODE_RUN);
        break;
    case PAGE_TEAMS:
        if (down & (KEY_UP | KEY_DOWN)) {
            int row = gMenuSelection / 7;
            int col = gMenuSelection % 7;
            row = down & KEY_UP ? (row + 4) % 5 : (row + 1) % 5;
            gMenuSelection = row * 7 + col;
            if (gMenuSelection >= ARENA2D_ROSTER_COUNT)
                gMenuSelection = ARENA2D_ROSTER_COUNT - 1;
        }
        if (down & KEY_LEFT) gMenuSelection = (gMenuSelection + ARENA2D_ROSTER_COUNT - 1) % ARENA2D_ROSTER_COUNT;
        if (down & KEY_RIGHT) gMenuSelection = (gMenuSelection + 1) % ARENA2D_ROSTER_COUNT;
        if (down & KEY_A) selectPlayerSpecies(0, gMenuSelection);
        if (down & KEY_START) {
            selectPlayerSpecies(0, gMenuSelection);
            startMode(MODE_QUICK);
        }
        break;
    case PAGE_SETTINGS:
        if (down & KEY_UP) gMenuSelection = (gMenuSelection + 2) % 3;
        if (down & KEY_DOWN) gMenuSelection = (gMenuSelection + 1) % 3;
        if (down & KEY_LEFT) adjustSetting(-1);
        if (down & (KEY_RIGHT | KEY_A)) adjustSetting(1);
        break;
    case PAGE_ONLINE:
    case PAGE_2PLAY:
    case PAGE_TUTORIAL:
    case PAGE_CREDITS:
        break;
    case PAGE_DEX:
        if (down & KEY_UP) gMenuSelection = (gMenuSelection + ARENA2D_ROSTER_COUNT - 1) % ARENA2D_ROSTER_COUNT;
        if (down & KEY_DOWN) gMenuSelection = (gMenuSelection + 1) % ARENA2D_ROSTER_COUNT;
        break;
    case PAGE_HOME:
    case PAGE_PLAY:
        break;
    }
}

static void drawPauseTop(void)
{
    drawBattleTop();
    rect(56, 68, 288, 104, 0x11182DF0);
    rect(56, 68, 288, 3, C_GOLD);
    fittedText(200, 88, 260, 0.94f, C_TEXT, "PAUSED", true);
    fittedText(200, 124, 260, 0.43f, C_MUTED, "CHOOSE AN ACTION BELOW", true);
}

static void drawPauseBottom(void)
{
    static const char *const actions[] = { "RESUME", "RESTART BATTLE", "MAIN MENU" };
    rect(0, 0, BOTTOM_W, BOTTOM_H, 0x121827FF);
    rect(0, 0, BOTTOM_W, 4, C_GOLD);
    textAt(12, 8, 0.60f, C_TEXT, "PAUSE MENU");
    for (int i = 0; i < 3; ++i)
        drawMenuRow(38.0f + i * 31.0f, actions[i], "", NULL,
                    gMenuSelection == i, false);
    textAt(12, 150, 0.42f, C_MUTED, "START OR B: RESUME");
}

static void handlePauseInput(u32 down)
{
    if (down & (KEY_B | KEY_START)) { gScene = SCENE_BATTLE; return; }
    if (down & KEY_UP) gMenuSelection = (gMenuSelection + 2) % 3;
    if (down & KEY_DOWN) gMenuSelection = (gMenuSelection + 1) % 3;
    if (!(down & KEY_A)) return;
    if (gMenuSelection == 0) gScene = SCENE_BATTLE;
    else if (gMenuSelection == 1) resetBattle();
    else { gScene = SCENE_TITLE; setMenuPage(PAGE_HOME); }
}

static void handleResultInput(u32 down)
{
    if (down & KEY_B) { gScene = SCENE_TITLE; setMenuPage(PAGE_HOME); return; }
    if (!(down & KEY_A)) return;
    if (gRoundWins[0] < 2 && gRoundWins[1] < 2) {
        ++gRoundNumber;
        resetBattle();
        return;
    }
    if (gGameMode == MODE_STORY) {
        if (gWinner == 0 && gStoryStage < 15) {
            ++gStoryStage;
            if (gStoryStage > gStoryUnlocked) gStoryUnlocked = gStoryStage;
            gRoundWins[0] = gRoundWins[1] = 0;
            gRoundNumber = 1;
            resetBattle();
        } else if (gWinner == 0) {
            gScene = SCENE_TITLE;
            setMenuPage(PAGE_STORY);
        } else {
            gRoundWins[0] = gRoundWins[1] = 0;
            gRoundNumber = 1;
            resetBattle();
        }
    } else if (gGameMode == MODE_RUN) {
        if (gWinner == 0 && gRunFloor < 6) {
            ++gRunFloor;
            gRoundWins[0] = gRoundWins[1] = 0;
            gRoundNumber = 1;
            resetBattle();
        } else {
            gScene = SCENE_TITLE;
            setMenuPage(PAGE_RUN);
            if (gWinner != 0) gRunFloor = 1;
        }
    } else {
        gRoundWins[0] = gRoundWins[1] = 0;
        gRoundNumber = 1;
        resetBattle();
    }
}

static void handleTouch(const touchPosition *touch)
{
    if (gScene == SCENE_TITLE) {
        if (gMenuPage != PAGE_HOME && touch->py >= 209) {
            if (gMenuPage == PAGE_TEAMS && touch->px < 160)
                startMode(MODE_QUICK);
            else
                goBackMenu();
            return;
        }
        if (gMenuPage == PAGE_HOME && touch->py >= 27 && touch->py < 202) {
            gMenuSelection = (touch->py - 27) / 25;
            if (gMenuSelection >= 0 && gMenuSelection < 7) activateHomeSelection();
        } else if (gMenuPage == PAGE_PLAY && touch->py >= 30 && touch->py < 192) {
            gMenuSelection = (touch->py - 30) / 27;
            if (gMenuSelection >= 0 && gMenuSelection < 6) activatePlaySelection();
        } else if (gMenuPage == PAGE_CPU) {
            if (touch->py >= 31 && touch->py < 112) {
                gMenuSelection = (touch->py - 31) / 27;
                int direction = touch->px < 160 ? -1 : 1;
                if (gMenuSelection == 0)
                    gPlayerSpecies[0] = (gPlayerSpecies[0] + ARENA2D_ROSTER_COUNT + direction) % ARENA2D_ROSTER_COUNT;
                else if (gMenuSelection == 1)
                    gOpponentSpecies = (gOpponentSpecies + ARENA2D_ROSTER_COUNT + direction) % ARENA2D_ROSTER_COUNT;
                else if (gMenuSelection == 2)
                    gCpuDifficulty = (gCpuDifficulty + 3 + direction) % 3;
            } else if (touch->py >= 120 && touch->py < 150) {
                gMenuSelection = 3;
                startMode(MODE_CPU);
            }
        } else if (gMenuPage == PAGE_STORY && touch->py >= 29 && touch->py < 191) {
            int first = (gStoryStage / 6) * 6;
            int stage = first + (touch->py - 29) / 27;
            if (stage < 16) {
                gStoryStage = stage;
                if (stage <= gStoryUnlocked) startMode(MODE_STORY);
            }
        } else if (gMenuPage == PAGE_RUN && touch->py >= 40 && touch->py < 66)
            startMode(MODE_RUN);
        else if (gMenuPage == PAGE_TEAMS) {
            if (touch->py >= 30 && touch->py < 200) {
                int row = (touch->py - 30) / 34;
                int col = touch->px / 45;
                int species = row * 7 + col;
                if (species >= 0 && species < ARENA2D_ROSTER_COUNT) {
                    gMenuSelection = species;
                    selectPlayerSpecies(0, species);
                }
            } else if (touch->py >= 216) startMode(MODE_QUICK);
        } else if (gMenuPage == PAGE_SETTINGS && touch->py >= 35 && touch->py < 122) {
            gMenuSelection = (touch->py - 35) / 29;
            if (gMenuSelection > 2) gMenuSelection = 2;
            adjustSetting(1);
        } else if (gMenuPage == PAGE_DEX && touch->py >= 29 && touch->py < 191) {
            int first = (gMenuSelection / 6) * 6;
            gMenuSelection = first + (touch->py - 29) / 27;
            if (gMenuSelection >= ARENA2D_ROSTER_COUNT)
                gMenuSelection = ARENA2D_ROSTER_COUNT - 1;
        }
        return;
    }
    if (gScene == SCENE_PAUSE) {
        if (touch->py >= 38 && touch->py < 131) {
            int choice = (touch->py - 38) / 31;
            if (choice == 0) gScene = SCENE_BATTLE;
            else if (choice == 1) resetBattle();
            else { gScene = SCENE_TITLE; setMenuPage(PAGE_HOME); }
        }
        return;
    }
    if (gScene == SCENE_RESULT) {
        if (touch->py < 150) handleResultInput(KEY_A);
        else handleResultInput(KEY_B);
        return;
    }
    if (gScene != SCENE_BATTLE) return;

    if (touch->py >= 85 && touch->py <= 162) {
        int column = touch->px / 64;
        if (column == 0) useBasic(0);
        else if (column == 1) useSkill(0);
        else if (column == 2) useSkill(1);
        else if (column == 3) useSkill(2);
        else useSkill(3);
    }
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    gfxInitDefault();
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 1;
    }
    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        C3D_Fini();
        gfxExit();
        return 1;
    }
    C2D_Prepare();
    (void)sfxInit();
    gMenuSheet = C2D_SpriteSheetLoadFromMem(
        menu_background_t3x_start,
        (size_t)(menu_background_t3x_end - menu_background_t3x_start));
    gTitleSheet = C2D_SpriteSheetLoadFromMem(
        web_title_t3x_start,
        (size_t)(web_title_t3x_end - web_title_t3x_start));
    gIconAtlas = C2D_SpriteSheetLoadFromMem(
        poke2d_icon_atlas_start,
        (size_t)(poke2d_icon_atlas_end - poke2d_icon_atlas_start));
    gFontAtlas = C2D_SpriteSheetLoadFromMem(
        poke2d_font_atlas_start,
        (size_t)(poke2d_font_atlas_end - poke2d_font_atlas_start));
    if (!gMenuSheet || !gTitleSheet || !gIconAtlas || !gFontAtlas ||
        !loadArenaTexture(gArenaIndex)) {
        if (gMenuSheet) C2D_SpriteSheetFree(gMenuSheet);
        if (gTitleSheet) C2D_SpriteSheetFree(gTitleSheet);
        if (gIconAtlas) C2D_SpriteSheetFree(gIconAtlas);
        if (gFontAtlas) C2D_SpriteSheetFree(gFontAtlas);
        if (gArenaSheet) C2D_SpriteSheetFree(gArenaSheet);
        sfxExit();
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 1;
    }
    gMenuBackground = C2D_SpriteSheetGetImage(gMenuSheet, 0);
    gWebTitle = C2D_SpriteSheetGetImage(gTitleSheet, 0);
    gGameLogo = C2D_SpriteSheetGetImage(
        gIconAtlas, poke2d_icon_indices_game_logo_idx);
    C3D_TexSetFilter(gMenuBackground.tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetFilter(gWebTitle.tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetFilter(gGameLogo.tex, GPU_NEAREST, GPU_NEAREST);
    for (int i = 0; i < P2D_TEXT_COUNT; ++i) {
        gTextFill[i] = C2D_SpriteSheetGetImage(gFontAtlas, p2dTextIndex[i]);
        C3D_TexSetFilter(gTextFill[i].tex, GPU_LINEAR, GPU_LINEAR);
    }
    gTop = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    gBottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!gTop || !gBottom) {
        C2D_SpriteSheetFree(gIconAtlas);
        C2D_SpriteSheetFree(gMenuSheet);
        C2D_SpriteSheetFree(gTitleSheet);
        C2D_SpriteSheetFree(gFontAtlas);
        if (gArenaSheet) C2D_SpriteSheetFree(gArenaSheet);
        sfxExit();
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 1;
    }

    u64 previous = osGetTime();
    while (aptMainLoop()) {
        u64 now = osGetTime();
        float dt = (float)(now - previous) / 1000.0f;
        previous = now;
        if (dt > 0.05f) dt = 0.05f;
        hidScanInput();
        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();
        circlePosition pad = { 0, 0 };
        hidCircleRead(&pad);
        touchPosition touch;
        bool touched = (down & KEY_TOUCH) != 0;
        if (touched) {
            hidTouchRead(&touch);
            handleTouch(&touch);
        }

        if (gScene == SCENE_TITLE)
            handleMenuInput(menuInputFromStick(down, &pad, dt));
        else if (gScene == SCENE_BATTLE) {
            gMenuStickDirection = 0;
            gMenuStickRepeat = 0.0f;
            updateBattle(dt, held, down, &pad);
        } else if (gScene == SCENE_PAUSE)
            handlePauseInput(menuInputFromStick(down, &pad, dt));
        else if (gScene == SCENE_RESULT)
            handleResultInput(menuInputFromStick(down, &pad, dt));

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(gTop, c2dColor(C_BG));
        C2D_SceneBegin(gTop);
        if (gScene == SCENE_TITLE) drawTitle();
        else if (gScene == SCENE_BATTLE) drawBattleTop();
        else if (gScene == SCENE_PAUSE) drawPauseTop();
        else drawResult();

        C2D_TargetClear(gBottom, c2dColor(C_BG));
        C2D_SceneBegin(gBottom);
        if (gScene == SCENE_TITLE) drawTitleBottom();
        else if (gScene == SCENE_BATTLE) drawBottomBattle();
        else if (gScene == SCENE_PAUSE) drawPauseBottom();
        else drawResultBottom();
        C3D_FrameEnd(0);
    }

    freeFighterAtlases();
    C2D_SpriteSheetFree(gIconAtlas);
    C2D_SpriteSheetFree(gMenuSheet);
    C2D_SpriteSheetFree(gTitleSheet);
    C2D_SpriteSheetFree(gFontAtlas);
    if (gArenaSheet) C2D_SpriteSheetFree(gArenaSheet);
    sfxExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
