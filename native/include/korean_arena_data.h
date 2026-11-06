/* Authorized Pokemon Arena source data, revision 8826a92. No music data. */
#ifndef KOREAN_ARENA_DATA_H
#define KOREAN_ARENA_DATA_H

#define ARENA2D_ROSTER_COUNT 6
#define ARENA2D_MOVE_COUNT 24

typedef struct {
    const char *key, *name, *type1, *type2, *basic;
    const char *moves[4];
    int dex, base[6], fallbackArt;
    bool legendary;
} Arena2dRosterEntry;

typedef struct {
    const char *key, *name, *type, *kind, *effect;
    int power; float cooldown, range;
} Arena2dMoveEntry;

static const Arena2dRosterEntry kArena2dRoster[ARENA2D_ROSTER_COUNT] = {
    {"charmander", "Charmander", "Fire", "", "Ember", {"charge", "flame_field", "dragon_pulse", "flamethrower"}, 4, {39, 52, 43, 60, 50, 65}, 1, false},
    {"squirtle", "Squirtle", "Water", "", "Water Gun", {"shell_charge", "whirlpool", "bubble_shield", "hydro_pump"}, 7, {44, 48, 65, 50, 64, 43}, 2, false},
    {"bulbasaur", "Bulbasaur", "Grass", "Poison", "Vine Whip", {"leech_seed", "petal_blizzard", "sleep_powder", "solar_beam"}, 1, {45, 49, 49, 65, 65, 45}, 0, false},
    {"pikachu", "Pikachu", "Electric", "", "Thunderbolt", {"quick_attack", "iron_tail", "electroweb", "thunder_dome"}, 25, {35, 55, 40, 50, 50, 90}, 3, false},
    {"scyther", "Scyther", "Bug", "Flying", "Fury Cutter", {"u_turn", "air_slash", "swords_dance", "double_team"}, 123, {70, 110, 80, 55, 80, 105}, 4, false},
    {"lucario", "Lucario", "Fighting", "Steel", "Bullet Punch", {"aura_sphere", "extreme_speed", "bone_boomerang", "bone_rush"}, 448, {70, 110, 70, 115, 70, 90}, 5, false},
};

static const Arena2dMoveEntry kArena2dMoves[ARENA2D_MOVE_COUNT] = {
    {"charge", "Flame Charge", "Fire", "dash", "burn", 70, 5.5f, 400.0f},
    {"flame_field", "Flame Field", "Fire", "quake", "burn", 58, 6.5f, 260.0f},
    {"dragon_pulse", "Dragon Pulse", "Dragon", "proj", "", 82, 8.5f, 560.0f},
    {"flamethrower", "Flamethrower", "Fire", "spread", "burn", 95, 11.0f, 540.0f},
    {"shell_charge", "Shell Charge", "Water", "dash", "", 68, 5.5f, 380.0f},
    {"whirlpool", "Whirlpool", "Water", "quake", "slow", 62, 6.5f, 280.0f},
    {"bubble_shield", "Bubble Shield", "Water", "mirror", "", 0, 8.5f, 0.0f},
    {"hydro_pump", "Hydro Pump", "Water", "proj", "", 108, 11.0f, 590.0f},
    {"leech_seed", "Leech Seed", "Grass", "psy", "slow", 48, 5.5f, 400.0f},
    {"petal_blizzard", "Petal Blizzard", "Grass", "spread", "", 75, 6.5f, 430.0f},
    {"sleep_powder", "Sleep Powder", "Grass", "proj", "sleep", 20, 8.5f, 370.0f},
    {"solar_beam", "Solar Beam", "Grass", "proj", "", 120, 11.0f, 600.0f},
    {"quick_attack", "Quick Attack", "Electric", "dash", "", 64, 4.8f, 420.0f},
    {"iron_tail", "Iron Tail", "Steel", "melee", "stun", 82, 5.2f, 135.0f},
    {"electroweb", "Electroweb", "Electric", "spread", "slow", 62, 8.0f, 390.0f},
    {"thunder_dome", "Thunder Dome", "Electric", "quake", "stun", 104, 12.0f, 300.0f},
    {"u_turn", "U-turn", "Bug", "dash", "", 70, 5.5f, 420.0f},
    {"air_slash", "Air Slash", "Flying", "spread", "", 75, 6.5f, 460.0f},
    {"swords_dance", "Swords Dance", "Normal", "buff", "", 0, 5.5f, 0.0f},
    {"double_team", "Double Team", "Normal", "mirror", "", 0, 14.0f, 0.0f},
    {"aura_sphere", "Aura Sphere", "Fighting", "homing", "", 80, 5.2f, 520.0f},
    {"extreme_speed", "Extreme Speed", "Normal", "dash", "", 78, 6.0f, 440.0f},
    {"bone_boomerang", "Bone Boomerang", "Ground", "spread", "", 84, 9.0f, 470.0f},
    {"bone_rush", "Bone Rush", "Fighting", "blink", "stun", 96, 12.0f, 460.0f},
};

#endif
