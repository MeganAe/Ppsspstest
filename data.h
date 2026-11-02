#ifndef DATA_H
#define DATA_H

/* Donnees du jeu, fidelement reprises de scripts/game.gd et scripts/player.gd du projet Godot. */

typedef enum {
    ET_PUMPKIN = 0, ET_SKULL, ET_RIBCAGE, ET_BOMBER, ET_GHOST, ET_TOTEM, ET_BRUTE, ET_BOSS, ET_COUNT
} EnemyTypeId;

typedef struct {
    const char *kind;
    float height, radius, speed;
    int damage, hp;
    float hover;        /* > 0 : vole a cette hauteur */
    int hop;            /* sautille en avançant */
    unsigned int glow;  /* couleur de lueur 0xAABBGGRR */
    int xp;
    float explode_radius; int explode_damage;
    int ranged, stationary, boss;
    float preferred_range;
} EType;

extern const EType ETYPES[ET_COUNT];

typedef struct {
    const char *name;
    int mag;
    float interval, reload;
    int damage, pellets;
    float spread_deg;
    int pierce, zoom, unlock; /* niveau de deblocage */
} WDef;

extern const WDef WDEFS[4];

/* kinds de pickups */
enum { PK_HEAL = 0, PK_SHIELD, PK_RAGE, PK_SWIFT, PK_GRENADE, PK_CANDLE, PK_COUNT };
extern const char *PICKUP_NAMES[PK_COUNT];
extern const unsigned int PICKUP_COLORS[PK_COUNT];

/* messages de la nuit : [fraction, texte] */
extern const float MSG_FRAC[4];
extern const char *MSG_TEXT[4];

#endif
