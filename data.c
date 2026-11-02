#include "data.h"
#include "util.h"

const EType ETYPES[ET_COUNT] = {
    /* pumpkin : citrouille orange qui sautille */
    {"pumpkin", 1.0f, 0.45f, 2.9f, 8, 2, 0.0f, 1, RGB(255, 107, 15), 10, 0.0f, 0, 0, 0, 0, 9.0f},
    /* skull : crane volant vert */
    {"skull", 0.8f, 0.45f, 4.3f, 10, 1, 1.1f, 0, RGB(102, 255, 140), 15, 0.0f, 0, 0, 0, 0, 9.0f},
    /* ribcage : ossements bleus, costauds */
    {"ribcage", 1.5f, 0.5f, 3.4f, 18, 4, 0.0f, 0, RGB(128, 230, 255), 25, 0.0f, 0, 0, 0, 0, 9.0f},
    /* bomber : citrouille jaune explosive */
    {"bomber", 0.95f, 0.45f, 4.6f, 6, 1, 0.0f, 1, RGB(255, 191, 26), 15, 2.7f, 22, 0, 0, 0, 9.0f},
    /* ghost : spectre volant qui tire a distance */
    {"ghost", 0.85f, 0.45f, 3.1f, 9, 3, 1.6f, 0, RGB(191, 217, 255), 20, 0.0f, 0, 1, 0, 0, 9.0f},
    /* totem : totem maudit immobile, tire de loin */
    {"totem", 1.9f, 0.5f, 0.0f, 12, 5, 0.0f, 0, RGB(184, 107, 255), 30, 0.0f, 0, 1, 1, 0, 13.0f},
    /* brute : grosse citrouille tanqueuse */
    {"brute", 1.45f, 0.65f, 2.0f, 20, 10, 0.0f, 1, RGB(255, 77, 13), 40, 0.0f, 0, 0, 0, 0, 9.0f},
    /* boss : le Roi Citrouille */
    {"boss", 2.6f, 0.95f, 2.15f, 25, 30, 0.0f, 1, RGB(255, 115, 13), 150, 0.0f, 0, 0, 0, 1, 9.0f},
};

const WDef WDEFS[4] = {
    {"Pistolet",            12, 0.22f, 1.1f, 1, 1, 0.0f,  0, 0, 1},
    {"Fusil d'assaut",      30, 0.095f, 1.6f, 1, 1, 1.4f, 0, 0, 2},
    {"Fusil a pompe",        6, 0.85f, 2.2f, 1, 6, 7.0f, 0, 0, 4},
    {"Fusil de precision",   5, 1.15f, 2.0f, 5, 1, 0.0f, 3, 1, 6},
};

const char *PICKUP_NAMES[PK_COUNT] = {"heal", "shield", "rage", "swift", "grenade", "candle"};
const unsigned int PICKUP_COLORS[PK_COUNT] = {
    RGB(255, 140, 38),   /* heal : citrouille orange */
    RGB(89, 166, 255),   /* shield : jaune/bleu */
    RGB(255, 64, 38),    /* rage : rouge */
    RGB(115, 255, 153),  /* swift : vert */
    RGB(140, 255, 128),  /* grenade : vert */
    RGB(140, 217, 255),  /* candle : bleu clair */
};

const float MSG_FRAC[4] = {0.2f, 0.45f, 0.7f, 0.9f};
const char *MSG_TEXT[4] = {
    "01:12 — Des cranes flottent dans la brume...",
    "02:42 — Les ossements se levent !",
    "04:12 — Tiens bon, l'aube approche...",
    "05:24 — Plus que quelques minutes !",
};
