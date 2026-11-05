#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "poke2d_sprite_indices.h"
#include "poke2d_sprite_meta.h"

#define TEAM_SIZE 3
#define TOP_W 400.0f
#define TOP_H 240.0f
#define BOTTOM_W 320.0f
#define BOTTOM_H 240.0f

typedef enum { SCENE_TITLE, SCENE_BATTLE, SCENE_PAUSE, SCENE_RESULT } Scene;
typedef enum { DROP_HEAL, DROP_CHARGE, DROP_SPEED } DropKind;
typedef enum { MODE_QUICK, MODE_CPU, MODE_STORY, MODE_RUN } GameMode;
typedef enum {
    PAGE_HOME, PAGE_CPU, PAGE_STORY, PAGE_RUN, PAGE_TEAMS, PAGE_ONLINE, PAGE_SETTINGS
} MenuPage;

typedef struct {
    const char *name;
    const char *type;
    int speciesArt;
    int maxHp;
    int hp;
} Pokemon;

typedef struct {
    float x, y;
    float moveX, moveY;
    float basicCd;
    float skillCd[3];
    float dashCd;
    float dashIFrames;
    float switchCd;
    float speedBoost;
    float ultimate;
    float hurtFlash;
    float attackAnim;
    float animClock;
    int animKind;
} Combatant;

typedef struct {
    float x, y, vx, vy, life;
    int damage;
    int active;
    int side;
    u32 color;
} Shot;

static const u32 kWhite = 0xFFF3D9FF;

static const Pokemon kSpecies[6] = {
    { "PIKACHU", "ELECTRIC", 0, 110, 110 },
    { "STEELIX", "STEEL / GROUND", 1, 180, 180 },
    { "SCIZOR", "BUG / STEEL", 2, 125, 125 },
    { "GYARADOS", "WATER / FLYING", 3, 170, 170 },
    { "CHARIZARD", "FIRE / FLYING", 4, 130, 130 },
    { "BLASTOISE", "WATER", 5, 160, 160 }
};
static Pokemon gTeam[2][TEAM_SIZE];
static const char *const kMoveLabel[6][3] = {
    { "CHARGE BEAM", "THUNDER PUNCH", "THUNDER WAVE" },
    { "IRON HEAD", "EARTHQUAKE", "DOUBLE-EDGE" },
    { "X-SCISSOR", "IRON HEAD", "DOUBLE-EDGE" },
    { "WATERFALL", "HURRICANE", "EARTHQUAKE" },
    { "FLAMETHROWER", "AIR SLASH", "DRAGON CLAW" },
    { "SURF", "ICE BEAM", "AURA SPHERE" }
};
static const char *const kUltimateLabel[6] = {
    "THUNDERBOLT", "DRAGON DANCE", "SWORDS DANCE", "DRAGON DANCE", "WILL-O-WISP", "PROTECT"
};

enum { ANIM_IDLE, ANIM_WALK, ANIM_SHOOT };

static C3D_RenderTarget *gTop;
static C3D_RenderTarget *gBottom;
static C2D_TextBuf gTextBuf;
static C2D_SpriteSheet gPoke2dAtlas;
static C2D_Image gVolcanicArena;
static C2D_Image gGrassArena;
static Scene gScene = SCENE_TITLE;
static MenuPage gMenuPage = PAGE_HOME;
static GameMode gGameMode = MODE_QUICK;
static Combatant gFighter[2];
static Shot gShots[8];
static int gActive[2] = { 0, 0 };
static int gWinner = -1;
static int gMenuSelection = 0;
static int gStoryStage = 0;
static int gStoryUnlocked = 0;
static int gRunFloor = 1;
static int gCpuDifficulty = 1;
static int gArenaIndex = 0;
static int gPlayerSpecies[TEAM_SIZE] = { 0, 1, 2 };
static int gTeamSlot = 0;
static float gUiTextScale = 1.15f;
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

static const u32 C_BG = 0x142330FF;
static const u32 C_TEXT = 0xF8F3D8FF;
static const u32 C_MUTED = 0xA7B2CBFF;
static const u32 C_GREEN = 0x57CB69FF;
static const u32 C_RED = 0xE75157FF;
static const u32 C_GOLD = 0xFFC53DFF;

extern const unsigned char poke2d_atlas_start[];
extern const unsigned char poke2d_atlas_end[];

__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global poke2d_atlas_start\n"
    "poke2d_atlas_start:\n"
    ".incbin \"../generated/poke2d/poke2d_atlas.t3x\"\n"
    ".global poke2d_atlas_end\n"
    "poke2d_atlas_end:\n"
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

static void textAt(float x, float y, float scale, u32 color, const char *s)
{
    C2D_Text text;
    C2D_TextParse(&text, gTextBuf, s);
    C2D_TextOptimize(&text);
    float actualScale = scale * 1.65f * gUiTextScale;
    C2D_DrawText(&text, C2D_WithColor, x, y, 0.45f,
                 actualScale, actualScale, c2dColor(color));
}

static void centeredText(float x, float y, float scale, u32 color, const char *s)
{
    C2D_Text text;
    C2D_TextParse(&text, gTextBuf, s);
    C2D_TextOptimize(&text);
    float actualScale = scale * 1.65f * gUiTextScale;
    float width = 0.0f;
    float height = 0.0f;
    C2D_TextGetDimensions(&text, actualScale, actualScale, &width, &height);
    (void)height;
    C2D_DrawText(&text, C2D_WithColor, x - width * 0.5f, y, 0.45f,
                 actualScale, actualScale, c2dColor(color));
}

static void fittedText(float x, float y, float maxWidth, float scale,
                       u32 color, const char *s, bool centered)
{
    C2D_Text text;
    C2D_TextParse(&text, gTextBuf, s);
    C2D_TextOptimize(&text);
    float actualScale = scale * 1.65f * gUiTextScale;
    float width = 0.0f;
    float height = 0.0f;
    C2D_TextGetDimensions(&text, actualScale, actualScale, &width, &height);
    (void)height;
    if (maxWidth > 0.0f && width > maxWidth) {
        actualScale *= maxWidth / width;
        width = maxWidth;
    }
    if (centered) x -= width * 0.5f;
    C2D_DrawText(&text, C2D_WithColor, x, y, 0.45f,
                 actualScale, actualScale, c2dColor(color));
}

static void drawBar(float x, float y, float w, float h, int hp, int maxHp)
{
    float ratio = maxHp > 0 ? clampf((float)hp / (float)maxHp, 0.0f, 1.0f) : 0.0f;
    rect(x, y, w, h, 0x101721FF);
    if (ratio > 0.0f)
        rect(x + 1.0f, y + 1.0f, (w - 2.0f) * ratio, h - 2.0f, ratio > 0.35f ? C_GREEN : C_RED);
}

static C2D_Image pokemonFrame(int art, int animation, int direction, int frame)
{
    if (art < 0 || art >= P2D_SPECIES_COUNT) art = 0;
    if (animation < 0 || animation >= P2D_ANIM_COUNT) animation = ANIM_IDLE;
    if (direction < 0 || direction >= P2D_DIR_COUNT) direction = 0;
    int count = p2dFrameCount[art][animation];
    if (count < 1) count = 1;
    if (frame < 0) frame = 0;
    frame %= count;
    return C2D_SpriteSheetGetImage(gPoke2dAtlas,
        p2dSpriteIndex[art][animation][direction][frame]);
}

static void drawPokemonAt(int art, int animation, int direction, int frame,
                          float pivotX, float pivotY)
{
    C2D_Image image = pokemonFrame(art, animation, direction, frame);
    float scale = 2.0f * p2dSpeciesScale[art];
    float originX = (float)p2dOriginX[art][animation] * scale;
    float originY = (float)p2dOriginY[art][animation] * scale;
    C2D_DrawImageAt(image, pivotX - originX, pivotY - originY, 0.36f,
                    NULL, scale, scale);
}

static void drawPokemonIcon(int art, float x, float y, float height)
{
    C2D_Image image = pokemonFrame(art, ANIM_IDLE, 0, 0);
    float sourceHeight = (float)p2dFrameHeight[art][ANIM_IDLE];
    float scale = sourceHeight > 0.0f ? height / sourceHeight : 1.0f;
    C2D_DrawImageAt(image, x, y, 0.18f, NULL, scale, scale);
}

static void drawStage(C2D_Image image)
{
    C2D_DrawImageAt(image, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
}

static int faceDirection(float dx, float dy)
{
    // The sprite rows are ordered down, down-left, left, up-left, up,
    // up-right, right, down-right.
    float angle = atan2f(dy, dx);
    int sector = (int)floorf(angle / 0.78539816339f + 0.5f);
    int row = (6 + sector) % 8;
    return row < 0 ? row + 8 : row;
}

static void resetBattle(void)
{
    static const int opponentOrder[6][3] = {
        { 3, 4, 5 }, { 4, 5, 3 }, { 5, 3, 4 },
        { 3, 5, 4 }, { 4, 3, 5 }, { 5, 4, 3 }
    };
    int retainedHp[TEAM_SIZE] = { 0, 0, 0 };
    bool carryRunHp = gGameMode == MODE_RUN && gRunFloor > 1 &&
                      gScene == SCENE_RESULT && gWinner == 0;
    if (carryRunHp)
        for (int i = 0; i < TEAM_SIZE; ++i)
            retainedHp[i] = gTeam[0][i].hp;
    int order = gGameMode == MODE_RUN ? (gRunFloor - 1) % 6 : 0;
    for (int i = 0; i < TEAM_SIZE; ++i) {
        gTeam[0][i] = kSpecies[gPlayerSpecies[i]];
        gTeam[0][i].hp = gTeam[0][i].maxHp;
        gTeam[1][i] = kSpecies[opponentOrder[order][i]];
        gTeam[1][i].hp = gTeam[1][i].maxHp;
        if (carryRunHp)
            gTeam[0][i].hp = retainedHp[i] < gTeam[0][i].maxHp ?
                             retainedHp[i] : gTeam[0][i].maxHp;
    }
    if (gGameMode == MODE_STORY) {
        gTeam[1][0] = kSpecies[3 + (gStoryStage % 3)];
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
    gFighter[0].x = 92.0f;
    gFighter[0].y = 155.0f;
    gFighter[1].x = 308.0f;
    gFighter[1].y = 130.0f;
    gFighter[0].ultimate = 50.0f;
    gFighter[1].ultimate = 50.0f;
    gBattleTime = 0.0f;
    static const float basicDelay[3] = { 1.10f, 0.90f, 0.67f };
    static const float skillDelay[3] = { 4.70f, 3.40f, 2.45f };
    gCpuBasicCd = basicDelay[gCpuDifficulty];
    gCpuSkillCd = skillDelay[gCpuDifficulty];
    gSupplyIn = 10.0f;
    gSupplyLife = 0.0f;
    gSupplyCount = 0;
    gHitFlash = 0.0f;
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
    gFighter[side].switchCd = 3.5f;
    gFighter[side].dashIFrames = 0.2f;
    if (side == 0) {
        gFighter[0].x = clampf(gFighter[0].x - 20.0f, 35.0f, 365.0f);
    } else {
        gFighter[1].x = clampf(gFighter[1].x + 20.0f, 35.0f, 365.0f);
    }
}

static void damagePokemon(int side, int damage, float chargeRatio)
{
    if (side == 0 && gFighter[0].dashIFrames > 0.0f)
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
        int next = nextLiving(side, gActive[side]);
        if (next >= 0) {
            gActive[side] = next;
            gFighter[side].switchCd = 0.0f;
            gFighter[side].dashIFrames = 0.78f;
        }
    }
}

static void playAttackAnimation(int side)
{
    gFighter[side].attackAnim = 0.44f;
    gFighter[side].animClock = 0.0f;
    gFighter[side].animKind = ANIM_SHOOT;
}

static void fireSkillShot(int side, int slot, int damage, float range)
{
    playAttackAnimation(side);
    int opponent = side == 0 ? 1 : 0;
    float dx = gFighter[opponent].x - gFighter[side].x;
    float dy = gFighter[opponent].y - gFighter[side].y;
    float mag = sqrtf(dx * dx + dy * dy);
    if (mag < 0.1f || mag > range) return;

    for (int i = 0; i < 8; ++i) {
        if (gShots[i].active) continue;
        int species = gTeam[side][gActive[side]].speciesArt;
        static const u32 colors[6][3] = {
            { 0xFFF04FFF, 0xF5C735FF, 0x9B79FFFF },
            { 0x65D878FF, 0xA8E657FF, 0xFFF07AFF },
            { 0x65C8F2FF, 0xA9E6F5FF, 0x4F9CFFFF },
            { 0xFF8A42FF, 0xF4C64EFF, 0xF94D38FF },
            { 0x4A9EFFFF, 0xED5062FF, 0x7DD8F2FF },
            { 0xB48BFFFF, 0xF2DDB6FF, 0xF3A85FFF }
        };
        gShots[i].x = gFighter[side].x;
        gShots[i].y = gFighter[side].y;
        gShots[i].vx = dx / mag * 245.0f;
        gShots[i].vy = dy / mag * 245.0f;
        gShots[i].life = 1.4f;
        gShots[i].damage = damage;
        gShots[i].color = colors[species][slot];
        gShots[i].active = 1;
        gShots[i].side = side;
        return;
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
        if (distance2(shot->x, shot->y, gFighter[target].x, gFighter[target].y) < 24.0f * 24.0f) {
            damagePokemon(target, shot->damage, 30.0f);
            gFighter[shot->side].ultimate = clampf(gFighter[shot->side].ultimate + 6.0f, 0.0f, 100.0f);
            shot->active = 0;
        } else if (shot->life <= 0.0f || shot->x < 12.0f || shot->x > 388.0f ||
                   shot->y < 42.0f || shot->y > 230.0f) {
            shot->active = 0;
        }
    }
}

static void useBasic(int side)
{
    Combatant *c = &gFighter[side];
    int opponent = side == 0 ? 1 : 0;
    if (c->basicCd > 0.0f || gScene != SCENE_BATTLE)
        return;
    c->basicCd = 0.5f;
    float d2 = distance2(c->x, c->y, gFighter[opponent].x, gFighter[opponent].y);
    if (d2 <= 70.0f * 70.0f) {
        playAttackAnimation(side);
        int targetMax = gTeam[opponent][gActive[opponent]].maxHp;
        int dmg = targetMax > 0 ? (int)(targetMax * 0.032f) : 4;
        if (dmg < 4) dmg = 4;
        damagePokemon(opponent, dmg, 30.0f);
        c->ultimate = clampf(c->ultimate + 3.0f, 0.0f, 100.0f);
    }
}

static void useSkill(int slot)
{
    Combatant *c = &gFighter[0];
    if (gScene != SCENE_BATTLE || slot < 0 || slot > 2 || c->skillCd[slot] > 0.0f)
        return;
    static const float cooldown[3] = { 3.2f, 4.0f, 5.0f };
    static const int damage[3] = { 12, 17, 22 };
    static const float range[3] = { 175.0f, 220.0f, 270.0f };
    c->skillCd[slot] = cooldown[slot];
    fireSkillShot(0, slot, damage[slot], range[slot]);
}

static void useDash(void)
{
    Combatant *c = &gFighter[0];
    if (c->dashCd > 0.0f) return;
    c->dashCd = 2.5f;
    c->dashIFrames = 0.3f;
    float dx = c->moveX;
    float dy = c->moveY;
    float mag = sqrtf(dx * dx + dy * dy);
    if (mag < 0.1f) {
        dx = c->x - gFighter[1].x;
        dy = c->y - gFighter[1].y;
        mag = sqrtf(dx * dx + dy * dy);
    }
    if (mag > 0.1f) {
        c->x = clampf(c->x + dx / mag * 38.0f, 28.0f, 372.0f);
        c->y = clampf(c->y + dy / mag * 32.0f, 52.0f, 215.0f);
    }
}

static void useUltimate(void)
{
    Combatant *c = &gFighter[0];
    if (c->ultimate < 100.0f) return;
    c->ultimate = 0.0f;
    if (distance2(c->x, c->y, gFighter[1].x, gFighter[1].y) <= 285.0f * 285.0f)
        damagePokemon(1, 45, 0.0f);
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
        gSupplyLife = 0.0f;
    }
    if (gSupplyLife < 0.0f) gSupplyLife = 0.0f;
}

static void updateBattle(float dt, u32 held, u32 down, const circlePosition *pad)
{
    if (down & KEY_START) {
        gMenuSelection = 0;
        gScene = SCENE_PAUSE;
        return;
    }
    gBattleTime += dt;
    updateShots(dt);
    Combatant *player = &gFighter[0];
    Combatant *cpu = &gFighter[1];
    float moveX = (float)pad->dx / 156.0f;
    float moveY = -(float)pad->dy / 156.0f;
    float mag = sqrtf(moveX * moveX + moveY * moveY);
    if (mag > 1.0f) { moveX /= mag; moveY /= mag; }
    player->moveX = moveX;
    player->moveY = moveY;
    float speed = player->speedBoost > 0.0f ? 125.0f : 96.0f;
    player->x = clampf(player->x + moveX * speed * dt, 26.0f, 374.0f);
    player->y = clampf(player->y + moveY * speed * dt, 96.0f, 184.0f);

    float dx = player->x - cpu->x;
    float dy = player->y - cpu->y;
    float d = sqrtf(dx * dx + dy * dy);
    static const float cpuSpeed[3] = { 23.0f, 34.0f, 47.0f };
    static const float basicDelay[3] = { 1.10f, 0.90f, 0.67f };
    static const float skillDelay[3] = { 4.70f, 3.40f, 2.45f };
    static const int basicDamage[3] = { 5, 7, 9 };
    static const int skillDamage[3] = { 10, 14, 18 };
    if (d > 58.0f && d > 0.1f) {
        cpu->x = clampf(cpu->x + dx / d * cpuSpeed[gCpuDifficulty] * dt, 26.0f, 374.0f);
        cpu->y = clampf(cpu->y + dy / d * cpuSpeed[gCpuDifficulty] * dt, 96.0f, 184.0f);
    } else {
        gCpuBasicCd -= dt;
        if (gCpuBasicCd <= 0.0f) {
            gCpuBasicCd = basicDelay[gCpuDifficulty];
            playAttackAnimation(1);
            damagePokemon(0, basicDamage[gCpuDifficulty], 30.0f);
        }
    }

    gCpuSkillCd -= dt;
    if (gCpuSkillCd <= 0.0f && d <= 250.0f) {
        int cpuSpecies = gTeam[1][gActive[1]].speciesArt;
        int slot = gActive[1] % 3;
        int damage = skillDamage[gCpuDifficulty] + (cpuSpecies == 4 ? 2 : 0);
        fireSkillShot(1, slot, damage, 250.0f);
        gCpuSkillCd = skillDelay[gCpuDifficulty] + (float)slot * 0.45f;
    }

    player->attackAnim = fmaxf(0.0f, player->attackAnim - dt);
    cpu->attackAnim = fmaxf(0.0f, cpu->attackAnim - dt);
    player->animKind = player->attackAnim > 0.0f ? ANIM_SHOOT :
                       (mag > 0.15f ? ANIM_WALK : ANIM_IDLE);
    cpu->animKind = cpu->attackAnim > 0.0f ? ANIM_SHOOT :
                    (d > 58.0f ? ANIM_WALK : ANIM_IDLE);
    player->animClock += dt;
    cpu->animClock += dt;
    player->basicCd = fmaxf(0.0f, player->basicCd - dt);
    player->dashCd = fmaxf(0.0f, player->dashCd - dt);
    player->switchCd = fmaxf(0.0f, player->switchCd - dt);
    player->dashIFrames = fmaxf(0.0f, player->dashIFrames - dt);
    player->speedBoost = fmaxf(0.0f, player->speedBoost - dt);
    player->hurtFlash = fmaxf(0.0f, player->hurtFlash - dt);
    cpu->dashIFrames = fmaxf(0.0f, cpu->dashIFrames - dt);
    cpu->hurtFlash = fmaxf(0.0f, cpu->hurtFlash - dt);
    for (int i = 0; i < 3; ++i)
        player->skillCd[i] = fmaxf(0.0f, player->skillCd[i] - dt);
    gHitFlash = fmaxf(0.0f, gHitFlash - dt);

    if ((held & KEY_A) || (down & KEY_A)) useBasic(0);
    if (down & KEY_B) useSkill(0);
    if (down & KEY_X) useSkill(1);
    if (down & KEY_Y) useSkill(2);
    if (down & KEY_L) useDash();
    if (down & KEY_R) useUltimate();
    if (down & KEY_LEFT) switchPokemon(0, (gActive[0] + TEAM_SIZE - 1) % TEAM_SIZE);
    if (down & KEY_RIGHT) switchPokemon(0, (gActive[0] + 1) % TEAM_SIZE);

    updateSupply(dt);
    if (livingCount(0) == 0 || livingCount(1) == 0) {
        gWinner = livingCount(0) > 0 ? 0 : 1;
        gScene = SCENE_RESULT;
    }
}

static void drawArena(void)
{
    drawStage(gArenaIndex == 0 ? gVolcanicArena : gGrassArena);
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
        drawPokemonIcon(gTeam[side][i].speciesArt, bx + 2.0f, y + 1.0f, 16.0f);
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
    float shadowW = pokemon->speciesArt == 1 || pokemon->speciesArt == 3 ? 30.0f : 22.0f;
    rect(fighter->x - shadowW * 0.5f, fighter->y + 2.0f,
         shadowW, 4.0f, 0x090D16AA);
    drawPokemonAt(pokemon->speciesArt, fighter->animKind, direction, frame,
                  fighter->x, fighter->y + bob);
    if (fighter->hurtFlash > 0.0f)
        rect(fighter->x - 10.0f, fighter->y - 31.0f, 20.0f, 2.0f, 0xFFF3D9FF);
    if (side == 0 && fighter->dashIFrames > 0.0f)
        C2D_DrawCircleSolid(fighter->x, fighter->y - 6.0f, 0.25f, 23.0f,
                            c2dColor(0xE9F8FF55));
}

static void drawBattleTop(void)
{
    drawArena();
    Pokemon *p = &gTeam[0][gActive[0]];
    Pokemon *e = &gTeam[1][gActive[1]];
    rect(4, 4, 143, 35, 0x25365DFF);
    rect(4, 4, 143, 2, 0x72A7F6FF);
    rect(253, 4, 143, 35, 0x25365DFF);
    rect(253, 4, 143, 2, 0xF06170FF);
    rect(8, 9, 26, 25, 0x101626FF);
    rect(359, 9, 26, 25, 0x101626FF);
    drawPokemonIcon(p->speciesArt, 9.0f, 10.0f, 22.0f);
    drawPokemonIcon(e->speciesArt, 360.0f, 10.0f, 22.0f);
    textAt(40, 7, 0.34f, C_TEXT, p->name);
    textAt(40, 18, 0.23f, C_MUTED, p->type);
    drawSegmentedBar(40, 28, 101, 7, p->hp, p->maxHp);
    textAt(268, 7, 0.34f, C_TEXT, e->name);
    textAt(268, 18, 0.23f, C_MUTED, e->type);
    drawSegmentedBar(268, 28, 86, 7, e->hp, e->maxHp);
    drawRoster(0, 7, 44);
    char clockText[16];
    int seconds = (int)gBattleTime;
    snprintf(clockText, sizeof(clockText), "%02d:%02d", seconds / 60, seconds % 60);
    rect(166, 4, 68, 34, 0x202C4AFF);
    centeredText(200, 8, 0.39f, C_TEXT, clockText);
    centeredText(200, 23, 0.23f, C_MUTED, "ROUND 1");

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
        rect(trailX - 3.0f, trailY - 2.0f, 8.0f, 3.0f, shot->color);
        C2D_DrawCircleSolid(shot->x, shot->y, 0.38f, 4.0f, c2dColor(shot->color));
        C2D_DrawCircleSolid(shot->x - 1.0f, shot->y - 1.0f, 0.42f, 1.5f,
                            c2dColor(kWhite));
    }
    drawFighter(0);
    drawFighter(1);
    if (gHitFlash > 0.0f)
        centeredText(200, 52, 0.31f, C_GOLD, "HIT!");
}

static void actionButton(float x, float y, const char *key, const char *label,
                         float cooldown, u32 tint)
{
    rect(x, y, 96.0f, 49.0f, 0x677083FF);
    rect(x + 2.0f, y + 2.0f, 92.0f, 45.0f, 0xF1C735FF);
    rect(x + 2.0f, y + 2.0f, 3.0f, 45.0f, tint);
    rect(x + 9.0f, y + 7.0f, 16.0f, 13.0f, 0x252A35FF);
    fittedText(x + 17.0f, y + 8.0f, 15.0f, 0.27f, C_TEXT, key, true);
    fittedText(x + 51.0f, y + 27.0f, 81.0f, 0.29f,
               0x202430FF, label, true);
    if (cooldown > 0.0f) {
        char cd[10];
        if (strcmp(key, "R") == 0)
            snprintf(cd, sizeof(cd), "%d%%", (int)gFighter[0].ultimate);
        else
            snprintf(cd, sizeof(cd), "%.1f", cooldown);
        fittedText(x + 73.0f, y + 8.0f, 18.0f, 0.23f,
                   0x4A4E58FF, cd, true);
    }
}

static void drawBottomBattle(void)
{
    rect(0, 0, BOTTOM_W, BOTTOM_H, 0x171B28FF);
    rect(0, 0, BOTTOM_W, 4, 0xF2C637FF);
    char matchHeader[48];
    const char *modeName = gGameMode == MODE_CPU ? "VS CPU" :
                           (gGameMode == MODE_STORY ? "STORY MODE" :
                            (gGameMode == MODE_RUN ? "ARENA RUN" : "QUICK BATTLE"));
    if (gGameMode == MODE_STORY)
        snprintf(matchHeader, sizeof(matchHeader), "%s  /  CHAPTER %02d",
                 modeName, gStoryStage + 1);
    else if (gGameMode == MODE_RUN)
        snprintf(matchHeader, sizeof(matchHeader), "%s  /  FLOOR %d OF 6",
                 modeName, gRunFloor);
    else
        snprintf(matchHeader, sizeof(matchHeader), "%s  /  %s ARENA", modeName,
                 gArenaIndex == 0 ? "VOLCANIC" : "GRASS");
    fittedText(9, 7, 302.0f, 0.40f, C_TEXT, matchHeader, false);
    for (int i = 0; i < TEAM_SIZE; ++i) {
        float x = 8.0f + i * 103.0f;
        Pokemon *member = &gTeam[0][i];
        rect(x, 23, 97, 43, i == gActive[0] ? 0x4A4029FF : 0x29334AFF);
        rect(x, 23, 97, 2, i == gActive[0] ? C_GOLD : 0x526180FF);
        drawPokemonIcon(member->speciesArt, x + 4.0f, 28.0f, 27.0f);
        textAt(x + 32.0f, 28.0f, 0.23f, C_TEXT, member->name);
        drawSegmentedBar(x + 32.0f, 45.0f, 61.0f, 7.0f, member->hp, member->maxHp);
        if (member->hp <= 0) rect(x + 1.0f, 62.0f, 95.0f, 2.0f, C_RED);
    }
    char ult[32];
    int species = gTeam[0][gActive[0]].speciesArt;
    snprintf(ult, sizeof(ult), "ULTIMATE  %d%%", (int)gFighter[0].ultimate);
    textAt(9, 73, 0.25f, C_TEXT, ult);
    drawBar(113, 76, 198, 8, (int)gFighter[0].ultimate, 100);
    actionButton(8, 91, "A", "BASIC", gFighter[0].basicCd, 0x75A8F8FF);
    actionButton(112, 91, "B", kMoveLabel[species][0], gFighter[0].skillCd[0], 0xF08C43FF);
    actionButton(216, 91, "X", kMoveLabel[species][1], gFighter[0].skillCd[1], 0xF2B93AFF);
    actionButton(8, 148, "Y", kMoveLabel[species][2], gFighter[0].skillCd[2], 0x9B85E9FF);
    actionButton(112, 148, "L", "SPACE DASH", gFighter[0].dashCd, 0x5BD0D6FF);
    actionButton(216, 148, "R", kUltimateLabel[species],
                 fmaxf(0.0f, 100.0f - gFighter[0].ultimate), 0xD28BEEFF);
    textAt(9, 207, 0.22f, C_MUTED, "CIRCLE PAD: MOVE   D-PAD: SWITCH POKEMON");
    textAt(9, 222, 0.20f, C_MUTED, "TAP A MOVE  |  L DODGE  |  R ULTIMATE");
}

static void drawTopMenuCard(const char *title, const char *subtitle,
                            const char *detail)
{
    drawStage(gGrassArena);
    rect(16, 16, 368, 208, 0x11182DEB);
    rect(16, 16, 368, 3, 0xF4C738FF);
    fittedText(30, 35, 340, 0.95f, C_TEXT, title, true);
    fittedText(34, 75, 332, 0.56f, C_GOLD, subtitle, true);
    fittedText(36, 104, 328, 0.43f, C_MUTED, detail, true);
    drawPokemonAt(0, ANIM_IDLE, 6, 0, 111.0f, 174.0f);
    drawPokemonAt(3, ANIM_IDLE, 2, 0, 289.0f, 174.0f);
    rect(64, 207, 272, 2, 0x53617FFF);
}

static void drawTitle(void)
{
    static const char *const titles[] = {
        "QUICK BATTLE", "VS CPU", "STORY MODE", "ARENA RUN",
        "YOUR TEAMS", "ONLINE", "SETTINGS"
    };
    static const char *const details[] = {
        "TRAINING MATCH  /  3 VS 3", "CHOOSE EASY, NORMAL OR HARD",
        "16 CHAPTERS  /  1 VS 1", "CLIMB SIX FLOORS",
        "EDIT YOUR THREE POKEMON", "LOCAL NETWORK BATTLES",
        "ARENA, CPU AND TEXT SIZE"
    };
    if (gMenuPage == PAGE_HOME) {
        drawTopMenuCard("POKE2D ARENA", "REAL-TIME POKEMON ARENA",
                        "SELECT A MODE ON THE LOWER SCREEN");
        fittedText(200, 126, 210, 0.63f, C_TEXT,
                   titles[gMenuSelection], true);
        fittedText(200, 151, 300, 0.39f, C_GOLD,
                   details[gMenuSelection], true);
        fittedText(200, 190, 280, 0.36f, C_TEXT,
                   "A OR TOUCH TO OPEN", true);
        return;
    }

    switch (gMenuPage) {
    case PAGE_CPU:
        drawTopMenuCard("VS CPU", "THREE POKEMON PER TEAM",
                        "MOVE WITH THE CIRCLE PAD  /  START TO FIGHT");
        fittedText(200, 134, 250, 0.70f, C_TEXT,
                   gCpuDifficulty == 0 ? "EASY CPU" :
                   (gCpuDifficulty == 1 ? "NORMAL CPU" : "HARD CPU"), true);
        break;
    case PAGE_STORY: {
        char stage[40];
        snprintf(stage, sizeof(stage), "CHAPTER %02d OF 16", gStoryStage + 1);
        drawTopMenuCard("STORY MODE", "SINGLE POKEMON DUELS", stage);
        fittedText(200, 134, 300, 0.58f, C_TEXT,
                   gStoryStage <= gStoryUnlocked ? "CHAPTER READY" : "CHAPTER LOCKED", true);
        break;
    }
    case PAGE_RUN: {
        char floor[40];
        snprintf(floor, sizeof(floor), "FLOOR %d OF 6", gRunFloor);
        drawTopMenuCard("ARENA RUN", "KEEP YOUR TEAM ALIVE", floor);
        fittedText(200, 134, 280, 0.56f, C_TEXT,
                   "A WIN MOVES YOU TO THE NEXT FLOOR", true);
        break;
    }
    case PAGE_TEAMS:
        drawTopMenuCard("YOUR TEAMS", "BUILD YOUR THREE POKEMON",
                        "SELECT A SLOT, THEN PICK FROM THE ROSTER");
        fittedText(200, 131, 280, 0.53f, C_TEXT,
                   kSpecies[gPlayerSpecies[gTeamSlot]].name, true);
        fittedText(200, 153, 280, 0.37f, C_GOLD,
                   "START OPENS A QUICK BATTLE", true);
        break;
    case PAGE_ONLINE:
        drawTopMenuCard("ONLINE", "LOBBY MENU", "NETWORK MATCHES NEED A REMOTE CLIENT");
        fittedText(200, 132, 320, 0.43f, C_TEXT,
                   "THIS BUILD PLAYS OFFLINE AGAINST THE CPU", true);
        break;
    case PAGE_SETTINGS:
        drawTopMenuCard("SETTINGS", "PERSONALIZE YOUR BATTLE", "LEFT / RIGHT CHANGES A VALUE");
        fittedText(200, 132, 310, 0.42f, C_TEXT,
                   "ARENA   CPU LEVEL   TEXT SIZE", true);
        break;
    case PAGE_HOME:
        break;
    }
}

static void drawMenuRow(float y, const char *title, const char *subtitle,
                        const char *status, bool selected, bool disabled)
{
    u32 fill = selected ? 0xF3A12EFF : 0x293452FF;
    u32 edge = selected ? 0xFFE17AFF : 0x4B5B7EFF;
    u32 mainColor = selected ? 0x17203AFF : (disabled ? 0x77829FFF : C_TEXT);
    u32 subColor = selected ? 0x40351FFF : 0xABB5D0FF;
    rect(8, y, 304, 23, fill);
    rect(8, y, 304, 1, edge);
    if (selected) rect(8, y, 4, 23, 0xFFF0A8FF);
    fittedText(17, y + 1, status ? 207.0f : 280.0f, 0.49f,
               mainColor, title, false);
    if (subtitle && subtitle[0])
        fittedText(17, y + 13, status ? 207.0f : 280.0f, 0.29f,
                   subColor, subtitle, false);
    if (status)
        fittedText(231, y + 6, 74.0f, 0.31f,
                   selected ? 0x273047FF : C_GOLD, status, true);
}

static void drawHomeMenuBottom(void)
{
    static const char *const titles[] = {
        "QUICK BATTLE", "VS CPU", "STORY MODE", "ARENA RUN",
        "YOUR TEAMS", "ONLINE", "SETTINGS"
    };
    static const char *const subtitles[] = {
        "TRAINING 3 VS 3", "PICK THE CPU DIFFICULTY", "CHOOSE A CHAPTER",
        "CLIMB SIX FLOORS", "BUILD YOUR TEAM", "OPEN THE LOBBY MENU",
        "ARENA AND DISPLAY OPTIONS"
    };
    rect(0, 0, BOTTOM_W, BOTTOM_H, 0x121827FF);
    rect(0, 0, BOTTOM_W, 4, 0xF2C637FF);
    textAt(12, 5, 0.55f, C_TEXT, "MAIN MENU");
    for (int i = 0; i < 7; ++i)
        drawMenuRow(27.0f + (float)i * 25.0f, titles[i], subtitles[i],
                    NULL, gMenuSelection == i, false);
    textAt(10, 211, 0.38f, C_GOLD, "A / TOUCH: SELECT");
    textAt(10, 225, 0.30f, C_MUTED, "START: QUICK BATTLE");
}

static void drawTitleBottom(void)
{
    rect(0, 0, BOTTOM_W, BOTTOM_H, 0x121827FF);
    rect(0, 0, BOTTOM_W, 4, 0xF2C637FF);
    if (gMenuPage == PAGE_HOME) {
        drawHomeMenuBottom();
        return;
    }

    switch (gMenuPage) {
    case PAGE_CPU:
        textAt(12, 7, 0.54f, C_TEXT, "CPU DIFFICULTY");
        drawMenuRow(34, "EASY", "SLOWER MOVES AND ATTACKS", "", gCpuDifficulty == 0, false);
        drawMenuRow(61, "NORMAL", "BALANCED CPU", "", gCpuDifficulty == 1, false);
        drawMenuRow(88, "HARD", "FASTER AND STRONGER CPU", "", gCpuDifficulty == 2, false);
        drawMenuRow(125, "START BATTLE", "USE THE SELECTED DIFFICULTY", "A", false, false);
        textAt(12, 164, 0.37f, C_MUTED, "TOUCH A LEVEL OR USE LEFT / RIGHT");
        textAt(12, 211, 0.38f, C_GOLD, "A: START  /  B: BACK");
        break;
    case PAGE_STORY: {
        int first = (gStoryStage / 6) * 6;
        textAt(12, 6, 0.53f, C_TEXT, "CHOOSE A CHAPTER");
        for (int row = 0; row < 6; ++row) {
            int stage = first + row;
            if (stage >= 16) break;
            char title[24], status[16];
            snprintf(title, sizeof(title), "CHAPTER %02d", stage + 1);
            snprintf(status, sizeof(status), "%s", stage <= gStoryUnlocked ?
                     (stage < gStoryUnlocked ? "CLEARED" : "READY") : "LOCKED");
            drawMenuRow(29.0f + (float)row * 27.0f, title,
                        "SINGLE DUEL", status, stage == gStoryStage,
                        stage > gStoryUnlocked);
        }
        textAt(10, 196, 0.34f, C_MUTED, "LEFT / RIGHT: PREVIOUS / NEXT PAGE");
        textAt(10, 216, 0.37f, C_GOLD, "A: PLAY  /  B: BACK");
        break;
    }
    case PAGE_RUN:
        textAt(12, 7, 0.54f, C_TEXT, "ARENA RUN");
        drawMenuRow(40, "START RUN", "SIX FIGHTS, ONE TEAM", "A", true, false);
        drawMenuRow(72, "TEAM CARRYOVER", "CURRENT HP CARRIES BETWEEN FLOORS", "", false, false);
        textAt(12, 119, 0.38f, C_MUTED, "WIN EACH FIGHT TO REACH THE NEXT FLOOR");
        textAt(12, 211, 0.38f, C_GOLD, "A: START  /  B: BACK");
        break;
    case PAGE_TEAMS: {
        textAt(12, 5, 0.54f, C_TEXT, "TEAM EDITOR");
        for (int i = 0; i < TEAM_SIZE; ++i) {
            float y = 27.0f + (float)i * 26.0f;
            bool selected = i == gTeamSlot;
            rect(8, y, 304, 23, selected ? 0xF3A12EFF : 0x293452FF);
            if (selected) rect(8, y, 4, 23, 0xFFF0A8FF);
            char label[36];
            snprintf(label, sizeof(label), "SLOT %d   %s", i + 1,
                     kSpecies[gPlayerSpecies[i]].name);
            fittedText(17, y + 3, 285, 0.48f,
                       selected ? 0x17203AFF : C_TEXT, label, false);
        }
        textAt(12, 108, 0.43f, C_GOLD, "ROSTER - TAP A POKEMON");
        for (int i = 0; i < 6; ++i) {
            int row = i / 2, col = i % 2;
            float x = col == 0 ? 8.0f : 164.0f;
            float y = 125.0f + (float)row * 25.0f;
            bool chosen = gPlayerSpecies[gTeamSlot] == i;
            rect(x, y, 148, 22, chosen ? 0x3E614BFF : 0x263149FF);
            drawPokemonIcon(i, x + 2.0f, y + 2.0f, 18.0f);
            fittedText(x + 29.0f, y + 3.0f, 112.0f, 0.40f,
                       chosen ? C_GOLD : C_TEXT, kSpecies[i].name, false);
        }
        textAt(10, 205, 0.33f, C_MUTED, "UP / DOWN: SLOT   LEFT / RIGHT: POKEMON");
        textAt(10, 222, 0.36f, C_GOLD, "START: FIGHT  /  B: BACK");
        break;
    }
    case PAGE_ONLINE:
        textAt(12, 7, 0.54f, C_TEXT, "ONLINE LOBBY");
        drawMenuRow(39, "HOST A ROOM", "CREATE A LINK BATTLE", "", false, true);
        drawMenuRow(67, "JOIN A ROOM", "ENTER A ROOM CODE", "", false, true);
        rect(9, 103, 302, 64, 0x252D42FF);
        fittedText(20, 113, 280, 0.42f, C_GOLD, "NETWORK PLAY IS NOT ACTIVE", false);
        fittedText(20, 135, 280, 0.31f, C_TEXT, "This build supports offline CPU battles.", false);
        textAt(12, 211, 0.38f, C_GOLD, "B: BACK");
        break;
    case PAGE_SETTINGS: {
        textAt(12, 7, 0.54f, C_TEXT, "SETTINGS");
        char arena[32], cpu[32], size[32];
        snprintf(arena, sizeof(arena), "ARENA: %s", gArenaIndex == 0 ? "VOLCANIC" : "GRASS");
        snprintf(cpu, sizeof(cpu), "CPU: %s", gCpuDifficulty == 0 ? "EASY" :
                 (gCpuDifficulty == 1 ? "NORMAL" : "HARD"));
        snprintf(size, sizeof(size), "TEXT SIZE: %s", gUiTextScale > 1.05f ? "LARGE" : "NORMAL");
        drawMenuRow(35, arena, "CHANGE BATTLE BACKGROUND", "LEFT / RIGHT", gMenuSelection == 0, false);
        drawMenuRow(64, cpu, "DEFAULT CPU LEVEL", "LEFT / RIGHT", gMenuSelection == 1, false);
        drawMenuRow(93, size, "MAKE LABELS LARGER", "LEFT / RIGHT", gMenuSelection == 2, false);
        rect(9, 132, 302, 54, 0x252D42FF);
        fittedText(18, 139, 284, 0.38f, C_GOLD, "CONTROLS", false);
        fittedText(18, 158, 284, 0.29f, C_TEXT, "PAD MOVE   A ATTACK   B/X/Y MOVES", false);
        fittedText(18, 173, 284, 0.29f, C_TEXT, "L DODGE   R ULTIMATE   START PAUSE", false);
        textAt(12, 211, 0.38f, C_GOLD, "A / LEFT / RIGHT: CHANGE  /  B: BACK");
        break;
    }
    case PAGE_HOME:
        break;
    }
}

static void drawResult(void)
{
    drawArena();
    rect(48, 70, 304, 94, 0x1C2743EE);
    rect(48, 70, 304, 3, gWinner == 0 ? C_GREEN : C_RED);
    fittedText(200, 82, 286, 0.94f, gWinner == 0 ? C_GREEN : C_RED,
               gWinner == 0 ? "VICTORY" : "DEFEAT", true);
    fittedText(200, 119, 280, 0.46f, C_TEXT, "A: CONTINUE   B: MAIN MENU", true);
}

static void drawResultBottom(void)
{
    rect(0, 0, BOTTOM_W, BOTTOM_H, C_BG);
    textAt(12, 18, 0.61f, C_TEXT, "MATCH COMPLETE");
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
        textAt(12, 56, 0.42f, C_MUTED, "A STARTS ANOTHER BATTLE");
    }
    textAt(12, 82, 0.42f, C_MUTED, "B RETURNS TO THE MAIN MENU");
}

static void setMenuPage(MenuPage page)
{
    gMenuPage = page;
    gMenuSelection = 0;
    if (page == PAGE_TEAMS) gTeamSlot = 0;
}

static void startMode(GameMode mode)
{
    gGameMode = mode;
    if (mode == MODE_RUN) gRunFloor = 1;
    resetBattle();
}

static void activateHomeSelection(void)
{
    switch (gMenuSelection) {
    case 0: startMode(MODE_QUICK); break;
    case 1: setMenuPage(PAGE_CPU); break;
    case 2: setMenuPage(PAGE_STORY); gStoryStage = gStoryUnlocked; break;
    case 3: setMenuPage(PAGE_RUN); gRunFloor = 1; break;
    case 4: setMenuPage(PAGE_TEAMS); break;
    case 5: setMenuPage(PAGE_ONLINE); break;
    case 6: setMenuPage(PAGE_SETTINGS); break;
    default: setMenuPage(PAGE_HOME); break;
    }
}

static void selectPlayerSpecies(int slot, int species)
{
    if (slot < 0 || slot >= TEAM_SIZE) return;
    int candidate = (species + 6) % 6;
    for (int tries = 0; tries < 6; ++tries) {
        bool used = false;
        for (int i = 0; i < TEAM_SIZE; ++i)
            if (i != slot && gPlayerSpecies[i] == candidate) used = true;
        if (!used) {
            gPlayerSpecies[slot] = candidate;
            return;
        }
        candidate = (candidate + 1) % 6;
    }
}

static void changePlayerSpecies(int slot, int delta)
{
    if (slot < 0 || slot >= TEAM_SIZE) return;
    selectPlayerSpecies(slot, gPlayerSpecies[slot] + delta);
}

static void adjustSetting(int delta)
{
    if (gMenuSelection == 0) gArenaIndex = (gArenaIndex + delta + 2) % 2;
    else if (gMenuSelection == 1)
        gCpuDifficulty = (gCpuDifficulty + delta + 3) % 3;
    else if (gMenuSelection == 2)
        gUiTextScale = gUiTextScale > 1.05f ? 1.0f : 1.15f;
}

static void handleMenuInput(u32 down)
{
    if (gScene != SCENE_TITLE) return;
    if (gMenuPage == PAGE_HOME) {
        if (down & KEY_UP) gMenuSelection = (gMenuSelection + 6) % 7;
        if (down & KEY_DOWN) gMenuSelection = (gMenuSelection + 1) % 7;
        if (down & KEY_START) { startMode(MODE_QUICK); return; }
        if (down & KEY_A) activateHomeSelection();
        return;
    }

    if (down & KEY_B) { setMenuPage(PAGE_HOME); return; }
    switch (gMenuPage) {
    case PAGE_CPU:
        if (down & KEY_LEFT) gCpuDifficulty = (gCpuDifficulty + 2) % 3;
        if (down & KEY_RIGHT) gCpuDifficulty = (gCpuDifficulty + 1) % 3;
        if (down & (KEY_A | KEY_START)) startMode(MODE_CPU);
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
        if (down & KEY_UP) gTeamSlot = (gTeamSlot + TEAM_SIZE - 1) % TEAM_SIZE;
        if (down & KEY_DOWN) gTeamSlot = (gTeamSlot + 1) % TEAM_SIZE;
        if (down & KEY_LEFT) changePlayerSpecies(gTeamSlot, -1);
        if (down & KEY_RIGHT) changePlayerSpecies(gTeamSlot, 1);
        if (down & KEY_A) gTeamSlot = (gTeamSlot + 1) % TEAM_SIZE;
        if (down & KEY_START) startMode(MODE_QUICK);
        break;
    case PAGE_SETTINGS:
        if (down & KEY_UP) gMenuSelection = (gMenuSelection + 2) % 3;
        if (down & KEY_DOWN) gMenuSelection = (gMenuSelection + 1) % 3;
        if (down & KEY_LEFT) adjustSetting(-1);
        if (down & (KEY_RIGHT | KEY_A)) adjustSetting(1);
        break;
    case PAGE_ONLINE:
        break;
    case PAGE_HOME:
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
    if (gGameMode == MODE_STORY) {
        if (gWinner == 0 && gStoryStage < 15) {
            ++gStoryStage;
            if (gStoryStage > gStoryUnlocked) gStoryUnlocked = gStoryStage;
            resetBattle();
        } else if (gWinner == 0) {
            gScene = SCENE_TITLE;
            setMenuPage(PAGE_STORY);
        } else resetBattle();
    } else if (gGameMode == MODE_RUN) {
        if (gWinner == 0 && gRunFloor < 6) {
            ++gRunFloor;
            resetBattle();
        } else {
            gScene = SCENE_TITLE;
            setMenuPage(PAGE_RUN);
            if (gWinner != 0) gRunFloor = 1;
        }
    } else resetBattle();
}

static void handleTouch(const touchPosition *touch)
{
    if (gScene == SCENE_TITLE) {
        if (gMenuPage == PAGE_HOME && touch->py >= 27 && touch->py < 202) {
            gMenuSelection = (touch->py - 27) / 25;
            if (gMenuSelection >= 0 && gMenuSelection < 7) activateHomeSelection();
        } else if (gMenuPage == PAGE_CPU) {
            if (touch->py >= 34 && touch->py < 111) {
                gCpuDifficulty = (touch->py - 34) / 27;
                startMode(MODE_CPU);
            } else if (touch->py >= 125 && touch->py < 150) startMode(MODE_CPU);
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
            if (touch->py >= 27 && touch->py < 105)
                gTeamSlot = (touch->py - 27) / 26;
            else if (touch->py >= 125 && touch->py < 200) {
                int row = (touch->py - 125) / 25;
                int col = touch->px >= 160 ? 1 : 0;
                int species = row * 2 + col;
                if (species >= 0 && species < 6) selectPlayerSpecies(gTeamSlot, species);
            } else if (touch->py >= 216) startMode(MODE_QUICK);
        } else if (gMenuPage == PAGE_SETTINGS && touch->py >= 35 && touch->py < 122) {
            gMenuSelection = (touch->py - 35) / 29;
            if (gMenuSelection > 2) gMenuSelection = 2;
            adjustSetting(1);
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

    if (touch->py >= 23 && touch->py <= 66) {
        int slot = touch->px / 103;
        if (slot >= 0 && slot < TEAM_SIZE) switchPokemon(0, slot);
        return;
    }
    if (touch->py >= 91 && touch->py <= 140) {
        if (touch->px < 108) useBasic(0);
        else if (touch->px < 212) useSkill(0);
        else useSkill(1);
    } else if (touch->py >= 148 && touch->py <= 197) {
        if (touch->px < 108) useSkill(2);
        else if (touch->px < 212) useDash();
        else useUltimate();
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
    gPoke2dAtlas = C2D_SpriteSheetLoadFromMem(
        poke2d_atlas_start, (size_t)(poke2d_atlas_end - poke2d_atlas_start));
    if (!gPoke2dAtlas) {
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 1;
    }
    gVolcanicArena = C2D_SpriteSheetGetImage(
        gPoke2dAtlas, poke2d_sprite_indices_arena_volcanic_idx);
    gGrassArena = C2D_SpriteSheetGetImage(
        gPoke2dAtlas, poke2d_sprite_indices_arena_grass_idx);
    C3D_TexSetFilter(gVolcanicArena.tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetFilter(gGrassArena.tex, GPU_NEAREST, GPU_NEAREST);
    gTextBuf = C2D_TextBufNew(8192);
    gTop = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    gBottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!gTextBuf || !gTop || !gBottom) {
        if (gTextBuf) C2D_TextBufDelete(gTextBuf);
        C2D_SpriteSheetFree(gPoke2dAtlas);
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

        if (gScene == SCENE_TITLE) handleMenuInput(down);
        else if (gScene == SCENE_BATTLE) updateBattle(dt, held, down, &pad);
        else if (gScene == SCENE_PAUSE) handlePauseInput(down);
        else if (gScene == SCENE_RESULT) handleResultInput(down);

        C2D_TextBufClear(gTextBuf);
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

    C2D_TextBufDelete(gTextBuf);
    C2D_SpriteSheetFree(gPoke2dAtlas);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
