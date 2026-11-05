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

static Pokemon gTeam[2][TEAM_SIZE] = {
    {
        { "PIKACHU", "ELECTRIC", 0, 110, 110 },
        { "STEELIX", "STEEL / GROUND", 1, 180, 180 },
        { "SCIZOR", "BUG / STEEL", 2, 125, 125 }
    },
    {
        { "GYARADOS", "WATER / FLYING", 3, 170, 170 },
        { "CHARIZARD", "FIRE / FLYING", 4, 130, 130 },
        { "BLASTOISE", "WATER", 5, 160, 160 }
    }
};
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
static Combatant gFighter[2];
static Shot gShots[8];
static int gActive[2] = { 0, 0 };
static int gWinner = -1;
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
    for (int side = 0; side < 2; ++side)
        for (int i = 0; i < TEAM_SIZE; ++i)
            gTeam[side][i].hp = gTeam[side][i].maxHp;

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
    gCpuBasicCd = 0.8f;
    gCpuSkillCd = 2.0f;
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
    if (d > 58.0f && d > 0.1f) {
        cpu->x = clampf(cpu->x + dx / d * 34.0f * dt, 26.0f, 374.0f);
        cpu->y = clampf(cpu->y + dy / d * 34.0f * dt, 96.0f, 184.0f);
    } else {
        gCpuBasicCd -= dt;
        if (gCpuBasicCd <= 0.0f) {
            gCpuBasicCd = 0.9f;
            playAttackAnimation(1);
            damagePokemon(0, 7, 30.0f);
        }
    }

    gCpuSkillCd -= dt;
    if (gCpuSkillCd <= 0.0f && d <= 250.0f) {
        int cpuSpecies = gTeam[1][gActive[1]].speciesArt;
        int slot = gActive[1] % 3;
        int damage = cpuSpecies == 4 ? 18 : 14;
        fireSkillShot(1, slot, damage, 250.0f);
        gCpuSkillCd = 3.4f + (float)slot * 0.5f;
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
    drawStage(gVolcanicArena);
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
    centeredText(x + 17.0f, y + 8.0f, 0.23f, C_TEXT, key);
    centeredText(x + 51.0f, y + 27.0f, 0.19f, 0x202430FF, label);
    if (cooldown > 0.0f) {
        char cd[10];
        if (strcmp(key, "R") == 0)
            snprintf(cd, sizeof(cd), "%d%%", (int)gFighter[0].ultimate);
        else
            snprintf(cd, sizeof(cd), "%.1f", cooldown);
        textAt(x + 74.0f, y + 8.0f, 0.18f, 0x4A4E58FF, cd);
    }
}

static void drawBottomBattle(void)
{
    rect(0, 0, BOTTOM_W, BOTTOM_H, 0x171B28FF);
    rect(0, 0, BOTTOM_W, 4, 0xF2C637FF);
    textAt(9, 7, 0.32f, C_TEXT, "QUICK BATTLE  /  VOLCANIC ARENA");
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

static void drawTitle(void)
{
    drawStage(gGrassArena);
    rect(24, 26, 352, 170, 0x14213BD8);
    rect(24, 26, 352, 3, 0xF4C738FF);
    centeredText(200, 48, 0.90f, C_TEXT, "POKE2D ARENA");
    centeredText(200, 78, 0.33f, C_GOLD, "QUICK BATTLE  /  TRAINING 3 VS 3");
    drawPokemonAt(0, ANIM_IDLE, 6, 0, 126.0f, 151.0f);
    drawPokemonAt(3, ANIM_IDLE, 2, 0, 276.0f, 151.0f);
    centeredText(200, 184, 0.31f, C_TEXT, "PRESS A TO PLAY");
}

static void drawTitleBottom(void)
{
    static const char *const modes[] = {
        "QUICK BATTLE  (TRAINING 3 VS 3)",
        "VS CPU  (PICK DIFFICULTY)",
        "STORY MODE", "ARENA RUN", "YOUR TEAMS", "ONLINE"
    };
    rect(0, 0, BOTTOM_W, BOTTOM_H, 0x171B28FF);
    rect(0, 0, BOTTOM_W, 4, 0xF2C637FF);
    textAt(12, 10, 0.42f, C_TEXT, "GAME MODES");
    for (int i = 0; i < 6; ++i) {
        float y = 36.0f + i * 27.0f;
        rect(9, y, 302, 23, i == 0 ? 0x46412EFF : 0x252D42FF);
        if (i == 0) rect(9, y, 3, 23, C_GOLD);
        textAt(17, y + 6, 0.25f, i == 0 ? C_TEXT : C_MUTED, modes[i]);
        if (i > 0) textAt(275, y + 6, 0.18f, 0x8791A7FF, "SOON");
    }
    textAt(12, 207, 0.24f, C_GOLD, "A / TAP QUICK BATTLE TO START");
    textAt(12, 223, 0.19f, C_MUTED, "LOCAL CPU TRAINING");
}

static void drawResult(void)
{
    drawArena();
    rect(48, 70, 304, 94, 0x1C2743EE);
    rect(48, 70, 304, 3, gWinner == 0 ? C_GREEN : C_RED);
    centeredText(200, 86, 0.72f, gWinner == 0 ? C_GREEN : C_RED,
                 gWinner == 0 ? "VICTORY" : "DEFEAT");
    centeredText(200, 119, 0.32f, C_TEXT, "A: REMATCH   B: GAME MODES");
}

static void drawResultBottom(void)
{
    rect(0, 0, BOTTOM_W, BOTTOM_H, 0x171B28FF);
    textAt(12, 18, 0.45f, C_TEXT, "MATCH COMPLETE");
    textAt(12, 57, 0.28f, C_MUTED, "A STARTS ANOTHER QUICK BATTLE");
    textAt(12, 84, 0.28f, C_MUTED, "B RETURNS TO THE MODE MENU");
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
        if ((touch->py >= 36 && touch->py <= 62) || touch->py > 205)
            resetBattle();
        return;
    }
    if (gScene == SCENE_RESULT) {
        if (touch->py < 110) resetBattle();
        else gScene = SCENE_TITLE;
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
    gTextBuf = C2D_TextBufNew(2048);
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

        if (gScene == SCENE_TITLE && (down & (KEY_A | KEY_START))) resetBattle();
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
    C2D_SpriteSheetFree(gPoke2dAtlas);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
