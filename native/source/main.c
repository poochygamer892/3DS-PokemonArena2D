#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define TEAM_SIZE 3
#define TOP_W 400.0f
#define TOP_H 240.0f
#define BOTTOM_W 320.0f
#define BOTTOM_H 240.0f

typedef enum { SCENE_TITLE, SCENE_BATTLE, SCENE_RESULT } Scene;
typedef enum { DROP_HEAL, DROP_CHARGE, DROP_SPEED } DropKind;

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
} Combatant;

typedef struct {
    float x, y, vx, vy, life;
    int damage;
    int active;
    u32 color;
} Shot;

static const u32 kBody[6] = {
    0xFFD83DFF, 0x72C979FF, 0x69BCEBFF,
    0xEF813FFF, 0x397ED1FF, 0xA96D48FF
};
static const u32 kAccent[6] = {
    0xFFF078FF, 0x4F9E57FF, 0xC98955FF,
    0xF7C545FF, 0xE44852FF, 0xF3DDB2FF
};
static const u32 kDark[6] = {
    0x493523FF, 0x28563EFF, 0x29475AFF,
    0x673C31FF, 0x244B77FF, 0x573B2EFF
};
static const u32 kOutline = 0x242637FF;
static const u32 kWhite = 0xFFF3D9FF;
static const u32 kRed = 0xE84D5BFF;

static Pokemon gTeam[2][TEAM_SIZE] = {
    {
        { "PIKACHU", "ELECTRIC", 0, 110, 110 },
        { "BULBASAUR", "GRASS", 1, 105, 105 },
        { "SQUIRTLE", "WATER", 2, 120, 120 }
    },
    {
        { "GYARADOS", "WATER", 4, 170, 170 },
        { "CHARIZARD", "FIRE", 3, 130, 130 },
        { "EEVEE", "NORMAL", 5, 100, 100 }
    }
};
static const char *const kMoveLabel[6][3] = {
    { "THUNDER", "QUICK", "IRON" },
    { "VINE", "RAZOR", "SEED" },
    { "WATER", "BUBBLE", "SHELL" },
    { "FLAME", "WING", "BLAST" },
    { "WATER", "BITE", "CRUNCH" },
    { "QUICK", "BITE", "SWIFT" }
};

static C3D_RenderTarget *gTop;
static C3D_RenderTarget *gBottom;
static C2D_TextBuf gTextBuf;
static Scene gScene = SCENE_TITLE;
static Combatant gFighter[2];
static Shot gShots[4];
static int gActive[2] = { 0, 0 };
static int gWinner = -1;
static float gBattleTime = 0.0f;
static float gCpuBasicCd = 0.0f;
static float gSupplyIn = 10.0f;
static float gSupplyX = 0.0f;
static float gSupplyY = 0.0f;
static float gSupplyLife = 0.0f;
static int gSupplyKind = DROP_HEAL;
static int gSupplyCount = 0;
static float gHitFlash = 0.0f;

static const u32 C_BG = 0x142330FF;
static const u32 C_PANEL = 0x202E4BFF;
static const u32 C_PANEL2 = 0x344766FF;
static const u32 C_TEXT = 0xF8F3D8FF;
static const u32 C_MUTED = 0xA7B2CBFF;
static const u32 C_GREEN = 0x57CB69FF;
static const u32 C_RED = 0xE75157FF;
static const u32 C_GOLD = 0xFFC53DFF;

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
    C2D_DrawText(&text, C2D_WithColor, x, y, 0.45f, scale, scale, c2dColor(color));
}

static void centeredText(float x, float y, float scale, u32 color, const char *s)
{
    C2D_Text text;
    C2D_TextParse(&text, gTextBuf, s);
    C2D_TextOptimize(&text);
    float width = 0.0f;
    float height = 0.0f;
    C2D_TextGetDimensions(&text, scale, scale, &width, &height);
    (void)height;
    C2D_DrawText(&text, C2D_WithColor, x - width * 0.5f, y, 0.45f, scale, scale, c2dColor(color));
}

static void drawBar(float x, float y, float w, float h, int hp, int maxHp)
{
    float ratio = maxHp > 0 ? clampf((float)hp / (float)maxHp, 0.0f, 1.0f) : 0.0f;
    rect(x, y, w, h, 0x101721FF);
    if (ratio > 0.0f)
        rect(x + 1.0f, y + 1.0f, (w - 2.0f) * ratio, h - 2.0f, ratio > 0.35f ? C_GREEN : C_RED);
}

static void spriteBox(float x, float y, float pixel, int sx, int sy, int w, int h, u32 color)
{
    rect(x + (float)sx * pixel, y + (float)sy * pixel,
         (float)w * pixel, (float)h * pixel, color);
}

static void spriteOutline(float x, float y, float pixel, int sx, int sy, int w, int h, u32 fill)
{
    spriteBox(x, y, pixel, sx, sy, w, h, kOutline);
    if (w > 2 && h > 2)
        spriteBox(x, y, pixel, sx + 1, sy + 1, w - 2, h - 2, fill);
}

static void drawCreature(int art, float x, float y, float pixel)
{
    art %= 6;
    u32 body = kBody[art];
    u32 light = kAccent[art];
    u32 shade = kDark[art];

    /* Each design uses a large silhouette and species-specific details so it
       stays recognizable on the 3DS screen at small sizes. */
    if (art == 0) { /* Pikachu: pointed ears, red cheeks and lightning tail. */
        spriteOutline(x, y, pixel, 13, 7, 3, 3, body);
        spriteOutline(x, y, pixel, 14, 5, 2, 3, body);
        spriteBox(x, y, pixel, 14, 5, 2, 1, shade);
        spriteOutline(x, y, pixel, 3, 0, 3, 7, body);
        spriteOutline(x, y, pixel, 10, 0, 3, 7, body);
        spriteBox(x, y, pixel, 3, 0, 3, 2, shade);
        spriteBox(x, y, pixel, 10, 0, 3, 2, shade);
        spriteOutline(x, y, pixel, 3, 4, 10, 8, body);
        spriteOutline(x, y, pixel, 5, 10, 7, 5, body);
        spriteBox(x, y, pixel, 4, 7, 2, 2, kOutline);
        spriteBox(x, y, pixel, 10, 7, 2, 2, kOutline);
        spriteBox(x, y, pixel, 3, 9, 2, 2, kRed);
        spriteBox(x, y, pixel, 11, 9, 2, 2, kRed);
        spriteBox(x, y, pixel, 7, 5, 2, 1, light);
        spriteBox(x, y, pixel, 6, 14, 2, 2, shade);
        spriteBox(x, y, pixel, 10, 14, 2, 2, shade);
        spriteBox(x, y, pixel, 12, 8, 2, 1, light);
    } else if (art == 1) { /* Bulbasaur: squat body and unmistakable seed bulb. */
        spriteOutline(x, y, pixel, 6, 1, 8, 8, shade);
        spriteBox(x, y, pixel, 7, 2, 6, 6, 0x79B94EFF);
        spriteBox(x, y, pixel, 5, 4, 3, 2, 0x79B94EFF);
        spriteBox(x, y, pixel, 12, 3, 3, 2, 0x79B94EFF);
        spriteBox(x, y, pixel, 9, 0, 2, 3, 0x79B94EFF);
        spriteOutline(x, y, pixel, 3, 8, 11, 6, body);
        spriteOutline(x, y, pixel, 1, 5, 9, 7, body);
        spriteBox(x, y, pixel, 2, 6, 2, 2, light);
        spriteBox(x, y, pixel, 4, 7, 1, 2, kOutline);
        spriteBox(x, y, pixel, 7, 7, 1, 2, kOutline);
        spriteBox(x, y, pixel, 3, 12, 2, 3, shade);
        spriteBox(x, y, pixel, 10, 12, 2, 3, shade);
        spriteBox(x, y, pixel, 5, 13, 2, 2, shade);
        spriteBox(x, y, pixel, 8, 13, 2, 2, shade);
    } else if (art == 2) { /* Squirtle: blue turtle, curled tail and shell. */
        spriteOutline(x, y, pixel, 8, 7, 7, 7, shade);
        spriteBox(x, y, pixel, 9, 8, 5, 5, 0xC98C58FF);
        spriteBox(x, y, pixel, 11, 8, 1, 5, 0x815039FF);
        spriteBox(x, y, pixel, 9, 10, 5, 1, 0x815039FF);
        spriteOutline(x, y, pixel, 3, 8, 8, 6, body);
        spriteOutline(x, y, pixel, 1, 4, 8, 7, body);
        spriteBox(x, y, pixel, 2, 5, 2, 2, light);
        spriteBox(x, y, pixel, 4, 6, 1, 2, kOutline);
        spriteBox(x, y, pixel, 7, 6, 1, 2, kOutline);
        spriteBox(x, y, pixel, 4, 9, 4, 2, kWhite);
        spriteBox(x, y, pixel, 3, 13, 2, 2, shade);
        spriteBox(x, y, pixel, 9, 13, 2, 2, shade);
        spriteOutline(x, y, pixel, 12, 12, 3, 3, body);
    } else if (art == 3) { /* Charizard: wings, horns and a flaming tail. */
        spriteOutline(x, y, pixel, 0, 4, 6, 9, shade);
        spriteOutline(x, y, pixel, 10, 4, 6, 9, shade);
        spriteBox(x, y, pixel, 1, 6, 4, 5, 0x5475C4FF);
        spriteBox(x, y, pixel, 11, 6, 4, 5, 0x5475C4FF);
        spriteBox(x, y, pixel, 2, 7, 2, 3, 0x8EB0EFFF);
        spriteBox(x, y, pixel, 12, 7, 2, 3, 0x8EB0EFFF);
        spriteOutline(x, y, pixel, 6, 6, 5, 8, body);
        spriteOutline(x, y, pixel, 5, 3, 6, 6, body);
        spriteBox(x, y, pixel, 5, 2, 2, 2, light);
        spriteBox(x, y, pixel, 9, 2, 2, 2, light);
        spriteBox(x, y, pixel, 7, 5, 1, 1, kOutline);
        spriteBox(x, y, pixel, 10, 5, 1, 1, kOutline);
        spriteBox(x, y, pixel, 7, 13, 2, 2, shade);
        spriteBox(x, y, pixel, 10, 13, 2, 2, shade);
        spriteBox(x, y, pixel, 10, 12, 3, 2, body);
        spriteBox(x, y, pixel, 13, 11, 2, 3, kRed);
        spriteBox(x, y, pixel, 14, 12, 1, 2, light);
    } else if (art == 4) { /* Gyarados: long arched body, crest, fins and whiskers. */
        spriteOutline(x, y, pixel, 7, 1, 7, 5, shade);
        spriteBox(x, y, pixel, 8, 2, 5, 3, body);
        spriteBox(x, y, pixel, 8, 0, 2, 2, kRed);
        spriteBox(x, y, pixel, 11, 0, 2, 2, kRed);
        spriteOutline(x, y, pixel, 6, 5, 5, 4, shade);
        spriteOutline(x, y, pixel, 3, 8, 6, 4, shade);
        spriteOutline(x, y, pixel, 6, 11, 6, 4, shade);
        spriteBox(x, y, pixel, 7, 6, 3, 2, body);
        spriteBox(x, y, pixel, 4, 9, 4, 2, body);
        spriteBox(x, y, pixel, 7, 12, 4, 2, body);
        spriteBox(x, y, pixel, 4, 6, 2, 2, kRed);
        spriteBox(x, y, pixel, 2, 7, 2, 1, kWhite);
        spriteBox(x, y, pixel, 2, 9, 2, 1, kWhite);
        spriteBox(x, y, pixel, 11, 5, 3, 2, kRed);
        spriteBox(x, y, pixel, 11, 13, 3, 2, kRed);
        spriteBox(x, y, pixel, 8, 3, 1, 1, kWhite);
    } else { /* Eevee: large ears, cream ruff and fluffy tail. */
        spriteOutline(x, y, pixel, 3, 0, 4, 7, shade);
        spriteOutline(x, y, pixel, 10, 0, 4, 7, shade);
        spriteBox(x, y, pixel, 4, 1, 2, 4, 0xE8BFA1FF);
        spriteBox(x, y, pixel, 11, 1, 2, 4, 0xE8BFA1FF);
        spriteOutline(x, y, pixel, 4, 4, 9, 8, body);
        spriteBox(x, y, pixel, 3, 9, 3, 3, light);
        spriteBox(x, y, pixel, 11, 9, 3, 3, light);
        spriteBox(x, y, pixel, 5, 11, 7, 4, shade);
        spriteBox(x, y, pixel, 5, 7, 1, 2, kOutline);
        spriteBox(x, y, pixel, 11, 7, 1, 2, kOutline);
        spriteBox(x, y, pixel, 8, 9, 2, 1, shade);
        spriteOutline(x, y, pixel, 12, 10, 4, 4, body);
        spriteBox(x, y, pixel, 14, 11, 2, 2, light);
        spriteBox(x, y, pixel, 6, 14, 2, 2, shade);
        spriteBox(x, y, pixel, 10, 14, 2, 2, shade);
    }
}

static void resetBattle(void)
{
    for (int side = 0; side < 2; ++side)
        for (int i = 0; i < TEAM_SIZE; ++i)
            gTeam[side][i].hp = gTeam[side][i].maxHp;

    gActive[0] = 0;
    gActive[1] = 0;
    memset(gFighter, 0, sizeof(gFighter));
    memset(gShots, 0, sizeof(gShots));
    gFighter[0].x = 92.0f;
    gFighter[0].y = 135.0f;
    gFighter[1].x = 308.0f;
    gFighter[1].y = 105.0f;
    gFighter[0].ultimate = 50.0f;
    gFighter[1].ultimate = 50.0f;
    gBattleTime = 0.0f;
    gCpuBasicCd = 0.8f;
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

static void fireSkillShot(int slot, int damage, float range)
{
    float dx = gFighter[1].x - gFighter[0].x;
    float dy = gFighter[1].y - gFighter[0].y;
    float mag = sqrtf(dx * dx + dy * dy);
    if (mag < 0.1f || mag > range) return;

    for (int i = 0; i < 4; ++i) {
        if (gShots[i].active) continue;
        int species = gTeam[0][gActive[0]].speciesArt;
        static const u32 colors[6][3] = {
            { 0xFFF04FFF, 0xF5C735FF, 0x9B79FFFF },
            { 0x65D878FF, 0xA8E657FF, 0xFFF07AFF },
            { 0x65C8F2FF, 0xA9E6F5FF, 0x4F9CFFFF },
            { 0xFF8A42FF, 0xF4C64EFF, 0xF94D38FF },
            { 0x4A9EFFFF, 0xED5062FF, 0x7DD8F2FF },
            { 0xB48BFFFF, 0xF2DDB6FF, 0xF3A85FFF }
        };
        gShots[i].x = gFighter[0].x;
        gShots[i].y = gFighter[0].y;
        gShots[i].vx = dx / mag * 245.0f;
        gShots[i].vy = dy / mag * 245.0f;
        gShots[i].life = 1.4f;
        gShots[i].damage = damage;
        gShots[i].color = colors[species][slot];
        gShots[i].active = 1;
        return;
    }
}

static void updateShots(float dt)
{
    for (int i = 0; i < 4; ++i) {
        Shot *shot = &gShots[i];
        if (!shot->active) continue;
        shot->x += shot->vx * dt;
        shot->y += shot->vy * dt;
        shot->life -= dt;
        if (distance2(shot->x, shot->y, gFighter[1].x, gFighter[1].y) < 20.0f * 20.0f) {
            damagePokemon(1, shot->damage, 30.0f);
            gFighter[0].ultimate = clampf(gFighter[0].ultimate + 6.0f, 0.0f, 100.0f);
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
    fireSkillShot(slot, damage[slot], range[slot]);
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
        { 70, 78 }, { 200, 72 }, { 330, 78 }, { 72, 180 },
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
    player->y = clampf(player->y + moveY * speed * dt, 50.0f, 216.0f);

    float dx = player->x - cpu->x;
    float dy = player->y - cpu->y;
    float d = sqrtf(dx * dx + dy * dy);
    if (d > 58.0f && d > 0.1f) {
        cpu->x = clampf(cpu->x + dx / d * 34.0f * dt, 26.0f, 374.0f);
        cpu->y = clampf(cpu->y + dy / d * 34.0f * dt, 50.0f, 216.0f);
    } else {
        gCpuBasicCd -= dt;
        if (gCpuBasicCd <= 0.0f) {
            gCpuBasicCd = 0.9f;
            damagePokemon(0, 7, 30.0f);
        }
    }

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
    rect(0, 0, TOP_W, TOP_H, 0x315D3FFF);
    rect(10, 44, 380, 186, 0x6CAF50FF);
    for (int row = 0; row < 12; ++row) {
        for (int col = 0; col < 20; ++col) {
            if ((row + col) % 3 == 0)
                rect(12.0f + col * 19.0f, 46.0f + row * 15.0f, 18.0f, 14.0f, 0x70B554FF);
        }
    }
    rect(10, 44, 380, 2, 0xF0F0D8FF);
    rect(10, 228, 380, 2, 0xF0F0D8FF);
    rect(10, 44, 2, 186, 0xF0F0D8FF);
    rect(388, 44, 2, 186, 0xF0F0D8FF);
    rect(199, 44, 2, 186, 0xF0F0D8FF);
    C2D_DrawCircleSolid(200, 137, 0.4f, 34, c2dColor(0xF0F0D8FF));
    C2D_DrawCircleSolid(200, 137, 0.3f, 31, c2dColor(0x6CAF50FF));
    rect(198, 134, 4, 6, 0xF0F0D8FF);
    rect(196, 94, 8, 2, 0xF0F0D8FF);
    rect(196, 178, 8, 2, 0xF0F0D8FF);
}

static void drawTeamMini(int side, float x, float y)
{
    for (int i = 0; i < TEAM_SIZE; ++i) {
        float bx = x + i * 18.0f;
        rect(bx, y, 15, 15, i == gActive[side] ? C_GOLD : C_PANEL2);
        rect(bx + 2, y + 2, 11, 11, kBody[gTeam[side][i].speciesArt]);
        if (gTeam[side][i].hp <= 0)
            rect(bx, y + 13, 15, 2, C_RED);
    }
}

static void drawBattleTop(void)
{
    drawArena();
    Pokemon *p = &gTeam[0][gActive[0]];
    Pokemon *e = &gTeam[1][gActive[1]];
    rect(5, 4, 148, 36, C_PANEL);
    rect(247, 4, 148, 36, C_PANEL);
    textAt(11, 7, 0.38f, C_TEXT, p->name);
    textAt(11, 20, 0.30f, C_MUTED, p->type);
    drawBar(81, 21, 65, 8, p->hp, p->maxHp);
    textAt(11, 30, 0.24f, C_TEXT, "PLAYER");
    textAt(253, 7, 0.38f, C_TEXT, e->name);
    textAt(253, 20, 0.30f, C_MUTED, e->type);
    drawBar(323, 21, 65, 8, e->hp, e->maxHp);
    textAt(253, 30, 0.24f, C_TEXT, "CPU");
    drawTeamMini(0, 8, 43);
    drawTeamMini(1, 343, 43);
    char clockText[16];
    int seconds = (int)gBattleTime;
    snprintf(clockText, sizeof(clockText), "%02d:%02d", seconds / 60, seconds % 60);
    rect(166, 3, 68, 35, C_PANEL);
    centeredText(200, 8, 0.42f, C_TEXT, clockText);
    centeredText(200, 22, 0.25f, C_MUTED, "ROUND 1");

    if (gSupplyLife > 0.0f) {
        u32 supplyColor = gSupplyKind == DROP_HEAL ? 0xF48CC8FF : (gSupplyKind == DROP_CHARGE ? 0xB884F4FF : C_GOLD);
        C2D_DrawCircleSolid(gSupplyX, gSupplyY, 0.35f, 9, c2dColor(0x18243AFF));
        C2D_DrawCircleSolid(gSupplyX, gSupplyY, 0.30f, 6, c2dColor(supplyColor));
    }
    for (int i = 0; i < 4; ++i) {
        Shot *shot = &gShots[i];
        if (!shot->active) continue;
        float trailX = shot->x - shot->vx * 0.035f;
        float trailY = shot->y - shot->vy * 0.035f;
        rect(trailX - 3.0f, trailY - 2.0f, 9.0f, 4.0f, c2dColor(shot->color));
        C2D_DrawCircleSolid(shot->x, shot->y, 0.38f, 7.0f, c2dColor(shot->color));
        C2D_DrawCircleSolid(shot->x - 1.0f, shot->y - 1.0f, 0.42f, 2.5f, c2dColor(kWhite));
    }
    float bob = sinf(gBattleTime * 5.0f) * 2.0f;
    drawCreature(p->speciesArt, gFighter[0].x - 32, gFighter[0].y - 32 + bob, 4.0f);
    drawCreature(e->speciesArt, gFighter[1].x - 32, gFighter[1].y - 32 - bob, 4.0f);
    if (gFighter[0].dashIFrames > 0.0f)
        C2D_DrawCircleSolid(gFighter[0].x, gFighter[0].y, 0.25f, 30, c2dColor(0xFFFFFF66));
    if (gHitFlash > 0.0f)
        centeredText(200, 48, 0.3f, C_GOLD, "HIT");
}

static void actionButton(float x, const char *key, const char *label, float cooldown, u32 tint)
{
    rect(x, 129, 48, 58, C_PANEL2);
    rect(x + 2, 131, 44, 54, C_BG);
    rect(x + 2, 131, 44, 3, tint);
    textAt(x + 4, 139, 0.30f, C_GOLD, key);
    textAt(x + 4, 157, 0.21f, C_TEXT, label);
    if (cooldown > 0.0f) {
        char cd[10];
        snprintf(cd, sizeof(cd), "%.1f", cooldown);
        textAt(x + 29, 139, 0.20f, C_MUTED, cd);
    } else {
        textAt(x + 30, 139, 0.20f, C_GREEN, "OK");
    }
}

static void drawBottomBattle(void)
{
    rect(0, 0, BOTTOM_W, BOTTOM_H, C_BG);
    rect(0, 0, BOTTOM_W, 5, 0xF2B33AFF);
    textAt(10, 12, 0.43f, C_TEXT, "YOUR TEAM");
    textAt(222, 12, 0.31f, C_MUTED, "D-PAD SWITCH");
    for (int i = 0; i < TEAM_SIZE; ++i) {
        float x = 9.0f + i * 103.0f;
        Pokemon *p = &gTeam[0][i];
        rect(x, 32, 96, 50, i == gActive[0] ? 0x6A5528FF : C_PANEL);
        rect(x + 3, 35, 20, 20, kBody[p->speciesArt]);
        drawCreature(p->speciesArt, x + 4, 36, 1.0f);
        textAt(x + 26, 37, 0.27f, C_TEXT, p->name);
        drawBar(x + 26, 56, 65, 7, p->hp, p->maxHp);
        char hp[16];
        snprintf(hp, sizeof(hp), "%d/%d", p->hp, p->maxHp);
        textAt(x + 26, 66, 0.21f, C_MUTED, hp);
    }
    char ult[24];
    snprintf(ult, sizeof(ult), "ULTIMATE %d%%", (int)gFighter[0].ultimate);
    textAt(10, 93, 0.34f, C_TEXT, ult);
    drawBar(126, 96, 181, 9, (int)gFighter[0].ultimate, 100);
    int species = gTeam[0][gActive[0]].speciesArt;
    actionButton(8, "A", "TACKLE", gFighter[0].basicCd, 0x7FB9FFFF);
    actionButton(60, "B", kMoveLabel[species][0], gFighter[0].skillCd[0], 0xE17E64FF);
    actionButton(112, "X", kMoveLabel[species][1], gFighter[0].skillCd[1], 0xE9A95AFF);
    actionButton(164, "Y", kMoveLabel[species][2], gFighter[0].skillCd[2], 0xAB87F2FF);
    actionButton(216, "L", "DODGE", gFighter[0].dashCd, 0x68DAE5FF);
    actionButton(268, "R", "ULT", fmaxf(0.0f, 100.0f - gFighter[0].ultimate), 0xC587F2FF);
    textAt(10, 199, 0.27f, C_MUTED, "CIRCLE PAD: MOVE   D-PAD: SWITCH");
    textAt(10, 215, 0.24f, C_MUTED, "A BASIC   B/X/Y MOVES   L DODGE   R ULT");
}

static void drawTitle(void)
{
    rect(0, 0, TOP_W, TOP_H, C_BG);
    rect(18, 18, 364, 204, 0x36593FFF);
    for (int i = 0; i < 16; ++i)
        rect(22 + i * 22, 22, 15, 3, 0x82C85BFF);
    centeredText(200, 48, 0.85f, C_TEXT, "POKEMON ARENA 2D");
    centeredText(200, 80, 0.43f, C_GOLD, "REAL-TIME 3V3 BATTLE");
    drawCreature(0, 94, 105, 5.0f);
    drawCreature(4, 226, 105, 5.0f);
    centeredText(200, 205, 0.38f, C_TEXT, "PRESS A TO START");
    rect(0, 0, 400, 3, C_GOLD);
}

static void drawTitleBottom(void)
{
    rect(0, 0, BOTTOM_W, BOTTOM_H, C_BG);
    textAt(14, 14, 0.55f, C_TEXT, "QUICK BATTLE");
    rect(10, 42, 300, 4, C_GOLD);
    textAt(14, 63, 0.35f, C_MUTED, "3 POKEMON PER TEAM");
    textAt(14, 88, 0.35f, C_MUTED, "MOVE WITH CIRCLE PAD");
    textAt(14, 113, 0.35f, C_MUTED, "A BASIC  B/X/Y SKILLS");
    textAt(14, 138, 0.35f, C_MUTED, "L DODGE  R ULTIMATE");
    textAt(14, 187, 0.32f, C_GOLD, "LOCAL CPU TRAINING");
}

static void drawResult(void)
{
    drawArena();
    rect(53, 72, 294, 92, C_PANEL);
    centeredText(200, 89, 0.72f, gWinner == 0 ? C_GREEN : C_RED, gWinner == 0 ? "YOU WIN" : "YOU LOSE");
    centeredText(200, 124, 0.38f, C_TEXT, "A: REMATCH   B: TITLE");
    drawCreature(gTeam[gWinner == 0 ? 0 : 1][gActive[gWinner == 0 ? 0 : 1]].speciesArt, 168, 168, 4.0f);
}

static void drawResultBottom(void)
{
    rect(0, 0, BOTTOM_W, BOTTOM_H, C_BG);
    textAt(12, 18, 0.50f, C_TEXT, "MATCH COMPLETE");
    textAt(12, 56, 0.34f, C_MUTED, "A STARTS ANOTHER BATTLE");
    textAt(12, 82, 0.34f, C_MUTED, "B RETURNS TO THE TITLE");
}

static void handleTouch(const touchPosition *touch)
{
    if (gScene == SCENE_TITLE) {
        if (touch->py > 160) resetBattle();
        return;
    }
    if (gScene != SCENE_BATTLE) return;

    if (touch->py >= 32 && touch->py <= 82) {
        int slot = touch->px / 103;
        if (slot >= 0 && slot < TEAM_SIZE) switchPokemon(0, slot);
        return;
    }
    if (touch->py >= 129 && touch->py <= 189) {
        if (touch->px < 60) useBasic(0);
        else if (touch->px < 112) useSkill(0);
        else if (touch->px < 164) useSkill(1);
        else if (touch->px < 216) useSkill(2);
        else if (touch->px < 268) useDash();
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
    gTextBuf = C2D_TextBufNew(2048);
    gTop = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    gBottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!gTextBuf || !gTop || !gBottom) {
        if (gTextBuf) C2D_TextBufDelete(gTextBuf);
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

        if (gScene == SCENE_TITLE && (down & KEY_A)) resetBattle();
        else if (gScene == SCENE_BATTLE) updateBattle(dt, held, down, &pad);
        else if (gScene == SCENE_RESULT) {
            if (down & KEY_A) resetBattle();
            if (down & KEY_B) gScene = SCENE_TITLE;
        }

        C2D_TextBufClear(gTextBuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(gTop, c2dColor(C_BG));
        C2D_SceneBegin(gTop);
        if (gScene == SCENE_TITLE) drawTitle();
        else if (gScene == SCENE_BATTLE) drawBattleTop();
        else drawResult();

        C2D_TargetClear(gBottom, c2dColor(C_BG));
        C2D_SceneBegin(gBottom);
        if (gScene == SCENE_TITLE) drawTitleBottom();
        else if (gScene == SCENE_BATTLE) drawBottomBattle();
        else drawResultBottom();
        C3D_FrameEnd(0);
    }

    C2D_TextBufDelete(gTextBuf);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
