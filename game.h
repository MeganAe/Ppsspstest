#ifndef GAME_H
#define GAME_H

#include "util.h"
#include "data.h"

#define ARENA 34.0f                 /* demi-cote de l'aire de jeu (m), comme world.gd */
#define NIGHT_DURATION 300.0f       /* minuit -> 6h en 5 minutes */
#define HEAL_AMOUNT 25
#define FREEZE_SECONDS 6.0f
#define PICKUP_RADIUS 1.6f
#define SHOT_RANGE 70.0f

#define MAX_ENEMIES 48
#define MAX_PICKUPS 16
#define MAX_PROJ 64
#define MAX_GRENADES 8
#define MAX_PARTICLES 320
#define MAX_TEXTS 48
#define MAX_OBST 260
#define MAX_DECOR 420
#define MAX_MSGS 4

typedef struct {
    V3 pos;
    float yaw;              /* direction du personnage */
    int weapon;
    int unlocked[4];
    int ammo, mag;
    int reloading; float reload_t;
    float shoot_cd, flash_t;
    int hp, max_hp;
    int shield; float shield_t;
    float rage_t, swift_t, invuln_t, hurt_flash;
    int rolling; float roll_t; V3 roll_dir;
    int grenades;
    int dead;
    float anim_t;
} Player;

typedef struct {
    int alive;
    int type;
    V3 pos;
    float hp; int max_hp;
    float speed; int damage;
    float radius, height;
    int hover; float base_y;
    int hop, ranged, stationary, elite, boss;
    float explode_radius; int explode_damage;
    int xp_value;
    unsigned int glow;
    float attack_cd, shoot_t, hit_flash, phase;
    float scale;
} Enemy;

typedef struct { int alive; int kind; V3 pos; float t; } Pickup;
typedef struct { int alive; V3 pos, vel; float life; int damage; unsigned int color; } Projectile;
typedef struct { int alive; V3 pos, vel; float fuse; } Grenade;
typedef struct { int alive; V3 pos, vel; float life, max_life, size; unsigned int color; int grav; } Particle;
typedef struct { int alive; V3 pos; float t; int amount; int crit; } FloatText;

/* obstacle de collision : cercle en XZ, pleine hauteur */
typedef struct { float x, z, r; } Obstacle;

/* decor visuel (pas de collision sauf si present dans obst[]) */
enum {
    DC_GRAVE = 0, DC_GRAVESTONE, DC_MARKER, DC_COFFIN, DC_TREE, DC_LANTERN,
    DC_CANDLE, DC_PUMPKIN, DC_BONE, DC_POST, DC_BENCH, DC_SHRINE, DC_FENCE_BROKEN, DC_COUNT
};
typedef struct { int kind; float x, z, yaw, scale; } Decor;

typedef struct { char text[96]; float t, dur; } Msg;

typedef struct {
    float ax, ay;                              /* mouvement -1..1 (relatif camera) */
    int shoot, reload, roll, grenade, wnext, wprev, start, sel;
    int up, down, left, right;                 /* fronts (menus) */
} Input;

typedef enum { ST_MENU, ST_PLAY, ST_PAUSE, ST_WON, ST_LOST } State;
typedef enum { MODE_NIGHT, MODE_WAVES } Mode;

typedef struct {
    State state;
    Mode mode;
    int menu_sel;          /* 0=nuit 1=vagues 2=difficulte */
    int difficulty;        /* 0 Histoire 1 Normal 2 Cauchemar */

    float time;            /* game_time */
    float frozen_until;
    int endless;

    /* mode vagues */
    int wave, wave_to_spawn, wave_spawned, wave_boss_spawned;
    float wave_spawn_t, wave_rest_t;
    int hordes_done[2];
    int boss_spawned;

    Player pl;
    V3 cam_pos, cam_look;
    float cam_yaw, cam_pitch, cam_shake;

    Enemy enemies[MAX_ENEMIES]; int n_enemies;
    Pickup pickups[MAX_PICKUPS]; int n_pickups;
    Projectile projs[MAX_PROJ]; int n_projs;
    Grenade grenades[MAX_GRENADES]; int n_grenades;
    Particle parts[MAX_PARTICLES]; int n_parts;
    FloatText texts[MAX_TEXTS]; int n_texts;
    Msg msgs[MAX_MSGS]; int n_msgs;

    float spawn_timer, power_timer, heal_timer, candle_timer;

    int xp, level, xp_next, kills, shots_fired, shots_hit, items_collected;
    int msg_index;
    float end_timer; int end_shown; int new_best;

    float best_time; int nights_won; int total_kills; int ach[8];

    Obstacle obst[MAX_OBST]; int n_obst;
    Decor decor[MAX_DECOR]; int n_decor;
    unsigned int decor_seed;

    float boom_flash;      /* flash d'explosion pour le rendu */
    V3 boom_pos;
    unsigned int rng_state;
} Game;

void game_init(Game *g);
void game_start(Game *g, Mode mode);
void game_update(Game *g, const Input *in, float dt);
void game_load_save(Game *g);
void game_save(Game *g);

/* utilitaires internes exposes pour les tests */
int  game_spawn_enemy(Game *g, int type, V3 pos);
void game_damage_player(Game *g, int dmg);
void game_add_xp(Game *g, int amount);
V3   game_random_open_point(Game *g, V3 avoid, float min_dist, float clear);
/* helper de test : tue tous les ennemis (simulation headless) */
void game_debug_kill_all(Game *g);

#endif
