/* La Nuit des Citrouilles — Edition PSP.
   Logique du jeu portee fidelement de scripts/game.gd, player.gd, enemy.gd, grenade.gd, pickup.gd. */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pspkernel.h>
#include <pspiofilemgr.h>

#include "game.h"
#include "audio.h"

/* ---------------------------------------------------------------- rng */
static float gr(Game *g) {
    unsigned int x = g->rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    g->rng_state = x ? x : 0x9E3779B9u;
    return (float)(x & 0xFFFFFF) / (float)0x1000000;
}
static int gri(Game *g, int n) { return n > 0 ? (int)(gr(g) * n) % n : 0; }
static float grange(Game *g, float a, float b) { return a + (b - a) * gr(g); }

/* ---------------------------------------------------------------- save */
#define SAVE_PATH "ms0:/PSP/GAME/CitroPSP/save.dat"

typedef struct {
    float best_time;
    int nights_won, total_kills;
    int ach[8];
    int difficulty;
} SaveData;

void game_load_save(Game *g) {
    g->best_time = 0; g->nights_won = 0; g->total_kills = 0;
    memset(g->ach, 0, sizeof(g->ach));
    g->difficulty = 1;
    SceUID fd = sceIoOpen(SAVE_PATH, PSP_O_RDONLY, 0777);
    if (fd >= 0) {
        SaveData s;
        if (sceIoRead(fd, &s, sizeof(s)) == (int)sizeof(s)) {
            g->best_time = s.best_time;
            g->nights_won = s.nights_won;
            g->total_kills = s.total_kills;
            memcpy(g->ach, s.ach, sizeof(g->ach));
            g->difficulty = s.difficulty;
        }
        sceIoClose(fd);
    }
}

void game_save(Game *g) {
    SaveData s;
    memset(&s, 0, sizeof(s));
    s.best_time = g->best_time;
    s.nights_won = g->nights_won;
    s.total_kills = g->total_kills;
    memcpy(s.ach, g->ach, sizeof(s.ach));
    s.difficulty = g->difficulty;
    sceIoMkdir("ms0:/PSP", 0777);
    sceIoMkdir("ms0:/PSP/GAME", 0777);
    sceIoMkdir("ms0:/PSP/GAME/CitroPSP", 0777);
    SceUID fd = sceIoOpen(SAVE_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, &s, sizeof(s));
        sceIoClose(fd);
    }
}

/* ---------------------------------------------------------------- monde */
static int is_free(Game *g, float x, float z, float clear) {
    for (int i = 0; i < g->n_obst; i++) {
        float dx = x - g->obst[i].x, dz = z - g->obst[i].z;
        float d = sqrtf(dx * dx + dz * dz);
        if (d < clear + g->obst[i].r) return 0;
    }
    return 1;
}

static void add_obst(Game *g, float x, float z, float r) {
    if (g->n_obst < MAX_OBST) {
        g->obst[g->n_obst].x = x; g->obst[g->n_obst].z = z; g->obst[g->n_obst].r = r;
        g->n_obst++;
    }
}

static void add_decor(Game *g, int kind, float x, float z, float yaw, float scale) {
    if (g->n_decor < MAX_DECOR) {
        Decor *d = &g->decor[g->n_decor++];
        d->kind = kind; d->x = x; d->z = z; d->yaw = yaw; d->scale = scale;
    }
}

static void scatter(Game *g, const int *kinds, int n_kinds, int count, float clear,
                    float r_obst, float smin, float smax, float margin, int obstacle) {
    int placed = 0, tries = 0;
    while (placed < count && tries < count * 40) {
        tries++;
        float x = grange(g, -ARENA + margin, ARENA - margin);
        float z = grange(g, -ARENA + margin, ARENA - margin);
        if (!is_free(g, x, z, clear)) continue;
        int k = kinds[gri(g, n_kinds)];
        add_decor(g, k, x, z, gr(g) * 6.2831853f, grange(g, smin, smax));
        if (obstacle) add_obst(g, x, z, r_obst);
        placed++;
    }
}

/* Genere le cimetiere (graines fixees comme world.gd : meme carte a chaque partie). */
static void build_world(Game *g) {
    g->n_obst = 0;
    g->n_decor = 0;
    g->rng_state = 20261031u;

    /* zones a garder degagees : depart, allee centrale, crypte */
    add_obst(g, 0.0f, 22.0f, 7.0f);
    for (int z = -18; z <= 24; z += 4) add_obst(g, 0.0f, (float)z, 2.6f);
    add_obst(g, 0.0f, -ARENA + 8.0f, 9.0f);

    /* tombes en grille */
    static const int graves[] = {DC_GRAVE, DC_GRAVE, DC_GRAVESTONE, DC_MARKER, DC_MARKER, DC_GRAVE};
    for (float x = -ARENA + 6.0f; x <= ARENA - 5.0f; x += 6.0f) {
        for (float z = -ARENA + 14.0f; z <= ARENA - 12.0f; z += 5.5f) {
            float px = x + grange(g, -1.1f, 1.1f);
            float pz = z + grange(g, -1.1f, 1.1f);
            if (gr(g) < 0.82f && is_free(g, px, pz, 1.6f)) {
                add_decor(g, graves[gri(g, 6)], px, pz, grange(g, -0.15f, 0.15f), 1.0f);
                add_obst(g, px, pz, 1.6f);
            }
        }
    }
    /* cercueils */
    { static const int coffins[] = {DC_COFFIN, DC_COFFIN};
      scatter(g, coffins, 2, 4, 2.2f, 2.2f, 1.0f, 1.0f, 5.0f, 1); }
    /* arbres morts dans l'arene */
    { static const int trees[] = {DC_TREE};
      scatter(g, trees, 1, 9, 5.0f, 1.0f, 0.85f, 1.1f, 5.0f, 1); }
    /* lanternes le long de l'allee centrale */
    for (int zi = 0; zi < 3; zi++) {
        float z = (float)(16 - zi * 12);
        for (int s = -1; s <= 1; s += 2) {
            float x = s * 3.6f;
            add_decor(g, DC_LANTERN, x, z, 0.0f, 1.0f);
            add_obst(g, x, z, 0.5f);
        }
    }
    { static const int lamps[] = {DC_LANTERN};
      scatter(g, lamps, 1, 7, 6.0f, 0.5f, 1.0f, 1.0f, 4.0f, 1); }
    { static const int candles[] = {DC_CANDLE};
      scatter(g, candles, 1, 6, 5.0f, 0.5f, 1.0f, 1.0f, 4.0f, 1); }
    /* citrouilles decoratives (les ennemis sont les citrouilles qui brillent et bougent) */
    { static const int pump[] = {DC_PUMPKIN};
      scatter(g, pump, 1, 26, 1.4f, 0.0f, 0.9f, 1.2f, 3.0f, 0); }
    /* os, poteaux, bancs, clotures cassees */
    { static const int bones[] = {DC_BONE};
      scatter(g, bones, 1, 12, 1.0f, 0.0f, 1.0f, 1.0f, 3.0f, 0); }
    { static const int posts[] = {DC_POST};
      scatter(g, posts, 1, 5, 2.0f, 1.2f, 1.0f, 1.0f, 4.0f, 1); }
    { static const int benches[] = {DC_BENCH};
      scatter(g, benches, 1, 3, 3.0f, 2.5f, 1.0f, 1.0f, 5.0f, 1); }
    { static const int fences[] = {DC_FENCE_BROKEN};
      scatter(g, fences, 1, 5, 2.0f, 1.5f, 1.0f, 1.0f, 4.0f, 1); }
    /* sanctuaires */
    { float sp[2][2] = {{-16.0f, -6.0f}, {17.0f, 8.0f}};
      for (int i = 0; i < 2; i++) {
          add_decor(g, DC_SHRINE, sp[i][0], sp[i][1], grange(g, -0.3f, 0.3f), 1.0f);
          add_obst(g, sp[i][0], sp[i][1], 2.5f);
      } }
    /* piliers du portail d'entree (sud) */
    add_obst(g, -1.45f, ARENA - 0.3f, 0.55f);
    add_obst(g, 1.45f, ARENA - 0.3f, 0.55f);

    /* foret d'arbres morts autour du cimetiere (visuel) */
    for (int i = 0; i < 110; i++) {
        float ring = grange(g, ARENA + 2.5f, ARENA + 24.0f);
        float along = grange(g, -ARENA - 24.0f, ARENA + 24.0f);
        int side = gri(g, 4);
        float x, z;
        switch (side) {
            case 0: x = along; z = -ring; break;
            case 1: x = along; z = ring; break;
            case 2: x = -ring; z = along; break;
            default: x = ring; z = along; break;
        }
        if (side == 1 && fabsf(x) < 6.0f && ring < ARENA + 12.0f) continue; /* entree degagee */
        add_decor(g, DC_TREE, x, z, gr(g) * 6.2831853f, grange(g, 0.9f, 1.4f));
    }
}

V3 game_random_open_point(Game *g, V3 avoid, float min_dist, float clear) {
    for (int i = 0; i < 60; i++) {
        V3 p = v3(grange(g, -ARENA + 3.0f, ARENA - 3.0f), 0.0f, grange(g, -ARENA + 3.0f, ARENA - 3.0f));
        if (v3distxz(p, avoid) >= min_dist && is_free(g, p.x, p.z, clear))
            return p;
    }
    return v3(grange(g, -10, 10), 0, grange(g, -10, 10));
}

/* ---------------------------------------------------------------- helpers */
static void add_msg(Game *g, const char *text, float dur) {
    if (g->n_msgs >= MAX_MSGS) {
        /* décale */
        for (int i = 0; i < MAX_MSGS - 1; i++) g->msgs[i] = g->msgs[i + 1];
        g->n_msgs = MAX_MSGS - 1;
    }
    Msg *m = &g->msgs[g->n_msgs++];
    strncpy(m->text, text, sizeof(m->text) - 1);
    m->text[sizeof(m->text) - 1] = 0;
    m->t = 0; m->dur = dur;
}

static void add_particle(Game *g, V3 pos, V3 vel, float life, float size, unsigned int color, int grav) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!g->parts[i].alive) {
            Particle *p = &g->parts[i];
            p->alive = 1; p->pos = pos; p->vel = vel;
            p->life = life; p->max_life = life; p->size = size;
            p->color = color; p->grav = grav;
            return;
        }
    }
}

static void add_text(Game *g, V3 pos, int amount, int crit) {
    for (int i = 0; i < MAX_TEXTS; i++) {
        if (!g->texts[i].alive) {
            FloatText *t = &g->texts[i];
            t->alive = 1; t->pos = pos; t->t = 0; t->amount = amount; t->crit = crit;
            return;
        }
    }
}

static void collide_world(Game *g, V3 *pos, float r) {
    float bound = ARENA - 0.5f - r;
    if (pos->x < -bound) pos->x = -bound;
    if (pos->x > bound) pos->x = bound;
    if (pos->z < -bound) pos->z = -bound;
    if (pos->z > bound) pos->z = bound;
    for (int i = 0; i < g->n_obst; i++) {
        float dx = pos->x - g->obst[i].x, dz = pos->z - g->obst[i].z;
        float d = sqrtf(dx * dx + dz * dz);
        float min = r + g->obst[i].r;
        if (d < min && d > 1e-4f) {
            pos->x = g->obst[i].x + dx / d * min;
            pos->z = g->obst[i].z + dz / d * min;
        }
    }
}

/* ---------------------------------------------------------------- combat */
static float diff_mult(Game *g, const float *table) {
    int d = g->difficulty; if (d < 0) d = 0; if (d > 2) d = 2;
    return table[d];
}

void game_damage_player(Game *g, int dmg) {
    Player *p = &g->pl;
    if (p->dead || p->invuln_t > 0.0f) return;
    if (p->shield > 0) {
        int absorbed = p->shield < dmg ? p->shield : dmg;
        p->shield -= absorbed;
        dmg -= absorbed;
        if (dmg <= 0) { p->invuln_t = 0.3f; return; }
    }
    p->hp -= dmg;
    p->invuln_t = 0.9f;
    p->hurt_flash = 1.0f;
    g->cam_shake = 0.7f;
    sfx_play(SFX_HURT);
    if (p->hp <= 0) {
        p->hp = 0;
        p->dead = 1;
        g->state = ST_LOST;
        g->end_timer = 1.8f;
        g->end_shown = 0;
        g->new_best = g->time > g->best_time;
        if (g->new_best) g->best_time = g->time;
        game_save(g);
        sfx_play(SFX_LOSE);
    }
}

static void kill_enemy(Game *g, int idx) {
    Enemy *e = &g->enemies[idx];
    if (!e->alive) return;
    e->alive = 0;
    g->kills++;
    g->total_kills++;
    game_add_xp(g, e->xp_value);
    if (g->total_kills >= 1 && !g->ach[0]) { g->ach[0] = 1; add_msg(g, "SUCCES DEBLOQUE — Premier sang", 3.5f); sfx_play(SFX_WIN); }
    if (g->total_kills >= 50 && !g->ach[1]) { g->ach[1] = 1; add_msg(g, "SUCCES DEBLOQUE — Chasseur de monstres (50)", 3.5f); sfx_play(SFX_WIN); }
    if (e->boss) {
        if (!g->ach[2]) { g->ach[2] = 1; add_msg(g, "SUCCES DEBLOQUE — Le Roi Citrouille est tombe", 3.5f); sfx_play(SFX_WIN); }
        g->cam_shake = 0.9f;
        g->boom_flash = 1.0f;
        g->boom_pos = e->pos;
    }
    /* effets de mort */
    for (int i = 0; i < 14; i++) {
        V3 v = v3(grange(g, -2.5f, 2.5f), grange(g, 0.5f, 3.5f), grange(g, -2.5f, 2.5f));
        add_particle(g, v3add(e->pos, v3(0, e->height * 0.5f, 0)), v, grange(g, 0.4f, 0.8f),
                     grange(g, 0.15f, 0.35f), e->glow, 1);
    }
    sfx_play(SFX_DIE);
    /* drops */
    if (e->boss) {
        V3 p1 = game_random_open_point(g, g->pl.pos, 3.0f, 1.0f);
        V3 p2 = game_random_open_point(g, g->pl.pos, 3.0f, 1.0f);
        /* spawn pickups directly */
        for (int i = 0; i < MAX_PICKUPS; i++)
            if (!g->pickups[i].alive) { g->pickups[i].alive = 1; g->pickups[i].kind = PK_SHIELD; g->pickups[i].pos = p1; g->pickups[i].t = 0; break; }
        for (int i = 0; i < MAX_PICKUPS; i++)
            if (!g->pickups[i].alive) { g->pickups[i].alive = 1; g->pickups[i].kind = PK_HEAL; g->pickups[i].pos = p2; g->pickups[i].t = 0; break; }
    } else if (gr(g) < 0.18f) {
        int kind = gr(g) < 0.6f ? PK_HEAL : (gr(g) < 0.5f ? PK_RAGE : PK_SWIFT);
        for (int i = 0; i < MAX_PICKUPS; i++)
            if (!g->pickups[i].alive) {
                g->pickups[i].alive = 1; g->pickups[i].kind = kind;
                g->pickups[i].pos = game_random_open_point(g, e->pos, 1.0f, 1.0f);
                g->pickups[i].t = 0;
                break;
            }
    }
}

static void damage_enemy(Game *g, int idx, int dmg, int crit) {
    Enemy *e = &g->enemies[idx];
    if (!e->alive) return;
    e->hp -= dmg;
    e->hit_flash = 0.12f;
    V3 hp_pos = v3(e->pos.x, e->pos.y + e->height * e->scale + 0.4f, e->pos.z);
    add_text(g, hp_pos, dmg, crit);
    sfx_play(SFX_HIT);
    if (e->hp <= 0) kill_enemy(g, idx);
}

/* intersection rayon / sphere, renvoie t ou -1 */
static float ray_sphere(V3 o, V3 d, V3 c, float r) {
    V3 oc = v3sub(o, c);
    float b = v3dot(oc, d);
    float cc = v3dot(oc, oc) - r * r;
    float disc = b * b - cc;
    if (disc < 0) return -1;
    float t = -b - sqrtf(disc);
    if (t < 0) t = -b + sqrtf(disc);
    return t < 0 ? -1 : t;
}

/* intersection rayon / cylindre vertical (obstacle), renvoie t ou -1 */
static float ray_cyl(Game *g, V3 o, V3 d, float max_t) {
    float best = max_t;
    for (int i = 0; i < g->n_obst; i++) {
        float cx = g->obst[i].x, cz = g->obst[i].z, r = g->obst[i].r;
        float a = d.x * d.x + d.z * d.z;
        if (a < 1e-6f) continue;
        float b = 2.0f * (o.x - cx) * d.x + 2.0f * (o.z - cz) * d.z;
        float c = (o.x - cx) * (o.x - cx) + (o.z - cz) * (o.z - cz) - r * r;
        float disc = b * b - 4 * a * c;
        if (disc < 0) continue;
        float t = (-b - sqrtf(disc)) / (2 * a);
        if (t > 0.01f && t < best) {
            float y = o.y + t * d.y;
            if (y > 0.0f && y < 3.5f) best = t;
        }
    }
    /* murs de l'arene */
    for (int w = 0; w < 4; w++) {
        float t;
        if (w == 0 && d.z < -1e-6f) t = (-ARENA - o.z) / d.z;
        else if (w == 1 && d.z > 1e-6f) t = (ARENA - o.z) / d.z;
        else if (w == 2 && d.x < -1e-6f) t = (-ARENA - o.x) / d.x;
        else if (w == 3 && d.x > 1e-6f) t = (ARENA - o.x) / d.x;
        else continue;
        if (t > 0.01f && t < best) {
            float y = o.y + t * d.y;
            float x = o.x + t * d.x, z = o.z + t * d.z;
            if (y > 0.0f && y < 12.0f && fabsf(x) < ARENA + 1 && fabsf(z) < ARENA + 1) best = t;
        }
    }
    return best;
}

static void fire_weapon(Game *g) {
    Player *p = &g->pl;
    const WDef *w = &WDEFS[p->weapon];
    if (p->ammo <= 0) {
        sfx_play(SFX_EMPTY);
        p->reloading = 1;
        p->reload_t = w->reload;
        return;
    }
    p->ammo--;
    p->shoot_cd = w->interval;
    p->flash_t = 0.06f;
    g->cam_shake = fmaxf(g->cam_shake, 0.08f);
    sfx_play(SFX_SHOOT);

    V3 chest = v3(p->pos.x, p->pos.y + 1.35f, p->pos.z);

    /* aide a la visee (comme le jeu original) : monstre le plus proche dans un cone devant */
    int target = -1;
    float best_ang = deg2rad(35.0f), best_d = SHOT_RANGE;
    V3 fwd = v3(sinf(p->yaw), 0, cosf(p->yaw));
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->alive) continue;
        V3 to = v3sub(e->pos, p->pos);
        float d = v3distxz(p->pos, e->pos);
        if (d > SHOT_RANGE) continue;
        V3 to_n = v3norm(v3(to.x, 0, to.z));
        float ang = acosf(clampf(v3dot(to_n, fwd), -1, 1));
        if (ang < best_ang || (fabsf(ang - best_ang) < 0.05f && d < best_d)) {
            best_ang = ang; best_d = d; target = i;
        }
    }
    V3 aim_point;
    if (target >= 0) {
        Enemy *e = &g->enemies[target];
        aim_point = v3(e->pos.x, e->pos.y + e->height * e->scale * 0.55f, e->pos.z);
    } else {
        aim_point = v3add(chest, v3scale(fwd, SHOT_RANGE));
        aim_point.y = 1.0f;
    }
    V3 shot_dir = v3norm(v3sub(aim_point, chest));

    g->shots_fired++;
    int pellets = w->pellets;
    float spread = deg2rad(w->spread_deg);
    for (int pel = 0; pel < pellets; pel++) {
        V3 dir = shot_dir;
        if (spread > 0) {
            dir = v3norm(v3add(shot_dir, v3scale(v3(grange(g, -1, 1), grange(g, -0.6f, 0.6f), grange(g, -1, 1)), spread * 10.0f)));
        }
        float max_t = ray_cyl(g, chest, dir, SHOT_RANGE);
        /* enemies touches, tries par distance */
        int hits[MAX_ENEMIES]; float thit[MAX_ENEMIES]; int nh = 0;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *e = &g->enemies[i];
            if (!e->alive) continue;
            V3 c = v3(e->pos.x, e->pos.y + e->height * e->scale * 0.55f, e->pos.z);
            float r = e->radius * e->scale + 0.12f;
            float t = ray_sphere(chest, dir, c, r);
            if (t > 0 && t < max_t) { hits[nh] = i; thit[nh] = t; nh++; }
        }
        /* tri simple */
        for (int a = 0; a < nh; a++)
            for (int b = a + 1; b < nh; b++)
                if (thit[b] < thit[a]) { int ti = hits[a]; hits[a] = hits[b]; hits[b] = ti;
                                         float tf = thit[a]; thit[a] = thit[b]; thit[b] = tf; }
        int max_hits = w->pierce > 0 ? w->pierce + 1 : 1;
        int any = 0;
        for (int h = 0; h < nh && h < max_hits; h++) {
            int dmg = w->damage;
            int crit = gr(g) < 0.12f;
            if (crit) dmg *= 2;
            if (p->rage_t > 0) dmg *= 2;
            damage_enemy(g, hits[h], dmg, crit);
            any = 1;
            g->shots_hit++;
            V3 hp_pt = v3add(chest, v3scale(dir, thit[h]));
            for (int k = 0; k < 4; k++)
                add_particle(g, hp_pt, v3(grange(g, -2, 2), grange(g, 0, 3), grange(g, -2, 2)),
                             0.3f, 0.12f, RGB(255, 220, 150), 1);
        }
        if (!any && max_t < SHOT_RANGE) {
            V3 hp_pt = v3add(chest, v3scale(dir, max_t));
            for (int k = 0; k < 3; k++)
                add_particle(g, hp_pt, v3(grange(g, -1, 1), grange(g, 1, 2), grange(g, -1, 1)),
                             0.25f, 0.1f, RGB(200, 200, 210), 1);
        }
        /* traceur */
        add_particle(g, v3add(chest, v3scale(dir, 1.0f)), v3scale(dir, 25.0f), 0.07f, 0.09f, RGB(255, 230, 160), 0);
    }
}

static void throw_grenade(Game *g) {
    Player *p = &g->pl;
    if (p->grenades <= 0) { sfx_play(SFX_EMPTY); return; }
    p->grenades--;
    sfx_play(SFX_THROW);
    for (int i = 0; i < MAX_GRENADES; i++) {
        if (!g->grenades[i].alive) {
            Grenade *gr_ = &g->grenades[i];
            gr_->alive = 1;
            V3 dir = v3(sinf(p->yaw), 0, cosf(p->yaw));
            gr_->pos = v3(p->pos.x + dir.x * 0.6f, p->pos.y + 1.45f, p->pos.z + dir.z * 0.6f);
            gr_->vel = v3(dir.x * 16.0f, 2.0f, dir.z * 16.0f);
            gr_->fuse = 0.9f;
            g->n_grenades++;
            return;
        }
    }
}

static void explode_grenade(Game *g, Grenade *gr_) {
    float radius = 6.5f, damage = 30.0f;
    g->boom_flash = 1.0f;
    g->boom_pos = gr_->pos;
    g->cam_shake = 1.0f;
    sfx_play(SFX_EXPLODE);
    for (int i = 0; i < 40; i++) {
        float a = gr(g) * 6.2831853f, sp = grange(g, 2, 14);
        add_particle(g, v3add(gr_->pos, v3(0, 0.6f, 0)),
                     v3(cosf(a) * sp, grange(g, 1, 8), sinf(a) * sp),
                     grange(g, 0.3f, 0.7f), grange(g, 0.2f, 0.5f), RGB(255, 140, 40), 1);
    }
    add_particle(g, v3add(gr_->pos, v3(0, 1.0f, 0)), v3(0, 0, 0), 0.35f, radius * 2.2f, RGBA(255, 180, 80, 160), 0);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->alive) continue;
        float d = v3distxz(gr_->pos, e->pos);
        float reach = radius + e->radius;
        if (d <= reach) {
            float falloff = d / reach;
            int dmg = (int)(damage * lerpf(1.0f, 0.3f, falloff));
            if (dmg < 1) dmg = 1;
            /* recul */
            V3 push = v3norm(v3(e->pos.x - gr_->pos.x, 0, e->pos.z - gr_->pos.z));
            e->pos = v3add(e->pos, v3scale(push, (reach - d) * 0.5f));
            damage_enemy(g, i, dmg, 0);
        }
    }
}

/* ---------------------------------------------------------------- spawns */
static int max_enemies(Game *g, float f) {
    static const float t[] = {0.65f, 1.0f, 1.35f};
    return (int)((8.0f + f * 26.0f) * diff_mult(g, t) + 0.5f);
}

static int pick_enemy_type(Game *g, float f) {
    int pool[24]; int n = 0;
    pool[n++] = ET_PUMPKIN; pool[n++] = ET_PUMPKIN; pool[n++] = ET_PUMPKIN;
    if (f >= 0.2f) { pool[n++] = ET_SKULL; pool[n++] = ET_SKULL; }
    if (f >= 0.35f) pool[n++] = ET_BOMBER;
    if (f >= 0.45f) { pool[n++] = ET_RIBCAGE; pool[n++] = ET_SKULL; }
    if (f >= 0.4f) pool[n++] = ET_TOTEM;
    if (f >= 0.5f) pool[n++] = ET_BRUTE;
    if (f >= 0.55f) pool[n++] = ET_GHOST;
    if (f >= 0.7f) { pool[n++] = ET_RIBCAGE; pool[n++] = ET_RIBCAGE; pool[n++] = ET_BOMBER; }
    return pool[gri(g, n)];
}

int game_spawn_enemy(Game *g, int type, V3 pos) {
    if (g->n_enemies >= MAX_ENEMIES) return -1;
    const EType *t = &ETYPES[type];
    Enemy *e = &g->enemies[g->n_enemies];
    memset(e, 0, sizeof(*e));
    e->alive = 1;
    e->type = type;
    e->pos = pos;
    float f = clampf(g->time / NIGHT_DURATION, 0, 1);
    static const float sp[] = {0.85f, 1.0f, 1.15f};
    static const float dm[] = {0.6f, 1.0f, 1.4f};
    static const float hm[] = {0.75f, 1.0f, 1.3f};
    float speed = t->speed;
    if (g->endless) speed *= 1.0f + fminf((g->time - NIGHT_DURATION) / 60.0f * 0.05f, 0.6f);
    e->speed = speed * diff_mult(g, sp);
    e->damage = (int)(t->damage * diff_mult(g, dm) + 0.5f); if (e->damage < 1) e->damage = 1;
    e->hp = (int)(t->hp * diff_mult(g, hm) + 0.5f); if (e->hp < 1) e->hp = 1;
    e->max_hp = e->hp;
    e->radius = t->radius; e->height = t->height;
    e->hover = t->hover > 0; e->base_y = t->hover;
    e->hop = t->hop; e->ranged = t->ranged; e->stationary = t->stationary;
    e->boss = t->boss;
    e->explode_radius = t->explode_radius; e->explode_damage = t->explode_damage;
    e->xp_value = t->xp;
    e->glow = t->glow;
    e->attack_cd = 0; e->shoot_t = 2.0f; e->phase = gr(g) * 6.2831853f;
    e->scale = 1.0f;
    /* elites dorees a partir du quart de la nuit */
    if (!e->boss && !e->stationary && f >= 0.25f && gr(g) < 0.12f) {
        e->elite = 1;
        e->hp *= 3; e->max_hp = e->hp;
        e->xp_value *= 3;
        e->glow = RGB(255, 217, 77);
        e->scale = 1.3f;
    }
    if (t->hover > 0) e->pos.y = t->hover; else e->pos.y = 0.15f;
    return g->n_enemies++;
}

static void spawn_enemy_at_random(Game *g, float f) {
    int type = pick_enemy_type(g, f);
    V3 p = game_random_open_point(g, g->pl.pos, 18.0f, 1.2f);
    game_spawn_enemy(g, type, p);
}

static void spawn_pickup(Game *g, int kind) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (!g->pickups[i].alive) {
            g->pickups[i].alive = 1;
            g->pickups[i].kind = kind;
            g->pickups[i].pos = game_random_open_point(g, g->pl.pos, 8.0f, 1.0f);
            g->pickups[i].t = 0;
            return;
        }
    }
}

static int count_pickups(Game *g, int kind) {
    int n = 0;
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (g->pickups[i].alive && g->pickups[i].kind == kind) n++;
    return n;
}

/* ---------------------------------------------------------------- xp / niveaux */
void game_add_xp(Game *g, int amount) {
    g->xp += amount;
    while (g->xp >= g->xp_next) {
        g->xp -= g->xp_next;
        g->level++;
        g->xp_next = 50 + (g->level - 1) * 40;
        /* montee de niveau : +10 PV et armes debloquees, sans pause */
        g->pl.hp = (int)fminf(g->pl.max_hp, g->pl.hp + 10);
        sfx_play(SFX_PICKUP);
        char buf[96];
        snprintf(buf, sizeof(buf), "NIVEAU %d ! +10 PV", g->level);
        for (int i = 0; i < 4; i++) {
            if (g->level >= WDEFS[i].unlock && !g->pl.unlocked[i]) {
                g->pl.unlocked[i] = 1;
                g->pl.weapon = i;
                g->pl.ammo = g->pl.mag = WDEFS[i].mag;
                g->pl.reloading = 0;
                snprintf(buf, sizeof(buf), "NIVEAU %d — NOUVELLE ARME : %s ! (L/R)", g->level, WDEFS[i].name);
                sfx_play(SFX_SWITCH);
                break;
            }
        }
        if (g->level >= 5 && !g->ach[3]) { g->ach[3] = 1; add_msg(g, "SUCCES DEBLOQUE — Niveau 5 atteint", 3.5f); sfx_play(SFX_WIN); }
        add_msg(g, buf, 3.5f);
    }
}

/* ---------------------------------------------------------------- init / start */
void game_init(Game *g) {
    memset(g, 0, sizeof(*g));
    g->state = ST_MENU;
    g->difficulty = 1;
    g->rng_state = 0xC17201u;
    build_world(g);
    game_load_save(g);
    g->cam_pitch = -0.32f;
    g->cam_yaw = (float)M_PI;
    g->menu_sel = 0;
    g->pl.pos = v3(0.0f, 0.1f, 22.0f);   /* le perso pose au portail pour le menu */
    g->pl.max_hp = 100;
    g->pl.hp = 100;
    g->cam_pos = v3(0, 4, 28);
    g->cam_look = v3(0, 1.3f, 22);
}

void game_start(Game *g, Mode mode) {
    State keep_state = g->state;
    (void)keep_state;
    int difficulty = g->difficulty;
    unsigned int decor_seed = 20261031u;
    (void)decor_seed;
    build_world(g);  /* re-genere le cimetiere (identique) */
    g->difficulty = difficulty;
    g->rng_state = 0xC17202u + (unsigned int)mode;

    g->state = ST_PLAY;
    g->mode = mode;
    g->time = 0;
    g->frozen_until = 0;
    g->endless = 0;
    g->wave = 0; g->wave_to_spawn = 0; g->wave_spawned = 0; g->wave_boss_spawned = 0;
    g->wave_spawn_t = 0; g->wave_rest_t = -1;
    g->hordes_done[0] = g->hordes_done[1] = 0;
    g->boss_spawned = 0;
    g->n_enemies = g->n_pickups = g->n_projs = g->n_grenades = g->n_parts = g->n_texts = g->n_msgs = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) g->enemies[i].alive = 0;
    for (int i = 0; i < MAX_PICKUPS; i++) g->pickups[i].alive = 0;
    for (int i = 0; i < MAX_PROJ; i++) g->projs[i].alive = 0;
    for (int i = 0; i < MAX_GRENADES; i++) g->grenades[i].alive = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) g->parts[i].alive = 0;
    for (int i = 0; i < MAX_TEXTS; i++) g->texts[i].alive = 0;

    g->spawn_timer = 2.0f;
    g->power_timer = 24.0f;
    g->heal_timer = 12.0f;
    g->candle_timer = 25.0f;

    g->xp = 0; g->level = 1; g->xp_next = 50;
    g->kills = 0; g->shots_fired = 0; g->shots_hit = 0; g->items_collected = 0;
    g->msg_index = 0;
    g->end_timer = 0; g->end_shown = 0; g->new_best = 0;
    g->boom_flash = 0;

    Player *p = &g->pl;
    memset(p, 0, sizeof(*p));
    p->pos = v3(0.0f, 0.1f, 22.0f);
    p->yaw = (float)M_PI;
    p->max_hp = 100; p->hp = 100;
    p->weapon = 0;
    p->unlocked[0] = 1;
    p->mag = WDEFS[0].mag;
    p->ammo = p->mag;
    p->grenades = 2;

    g->cam_yaw = (float)M_PI;
    g->cam_pos = v3(0, 4, 28);
    g->cam_look = v3(0, 1.3f, 22);

    if (mode == MODE_WAVES) {
        add_msg(g, "MODE VAGUES — Survis aux 10 vagues !", 4.0f);
        g->wave = 1;
        g->wave_to_spawn = 5 + 1 * 4;
        g->wave_spawn_t = 0.5f;
        add_msg(g, "VAGUE 1 / 10", 3.0f);
    } else {
        add_msg(g, "MINUIT — Survis jusqu'a l'aube !", 4.0f);
    }
    sfx_play(SFX_BELL);
    spawn_pickup(g, PK_HEAL);
    spawn_pickup(g, PK_HEAL);
}

/* ---------------------------------------------------------------- waves */
static void start_wave(Game *g, int n) {
    g->wave = n;
    g->wave_to_spawn = 5 + n * 4;
    g->wave_spawned = 0;
    g->wave_spawn_t = 0.5f;
    g->wave_rest_t = -1;
    g->wave_boss_spawned = 0;
    char buf[64];
    snprintf(buf, sizeof(buf), "VAGUE %d / 10", n);
    add_msg(g, buf, 3.0f);
    sfx_play(SFX_BELL);
}

static void update_waves(Game *g, float dt) {
    if (g->wave_rest_t > 0.0f) {
        g->wave_rest_t -= dt;
        if (g->wave_rest_t <= 0.0f) start_wave(g, g->wave + 1);
        return;
    }
    if (g->wave_to_spawn > 0) {
        g->wave_spawn_t -= dt;
        if (g->wave_spawn_t <= 0.0f && g->n_enemies < (6 + g->wave < 14 ? 6 + g->wave : 14)) {
            g->wave_spawn_t = 1.15f;
            g->wave_to_spawn--;
            g->wave_spawned++;
            float f = clampf((float)g->wave / 10.0f, 0.05f, 1.0f);
            if (g->wave >= 10 && !g->wave_boss_spawned && g->wave_spawned >= (5 + g->wave * 4) / 2) {
                g->wave_boss_spawned = 1;
                V3 p = game_random_open_point(g, g->pl.pos, 15.0f, 1.2f);
                game_spawn_enemy(g, ET_BOSS, p);
                add_msg(g, "LE ROI CITROUILLE SE LEVE !", 4.0f);
                sfx_play(SFX_BELL);
            } else {
                spawn_enemy_at_random(g, f);
            }
        }
    } else if (g->n_enemies == 0) {
        if (g->wave >= 10) {
            /* victoire */
            g->state = ST_WON;
            g->end_shown = 1;
            if (!g->ach[5]) { g->ach[5] = 1; }
            g->n_enemies = 0;
            sfx_play(SFX_WIN);
            game_save(g);
        } else {
            g->wave_rest_t = 12.0f;
            g->pl.hp = (int)fminf(g->pl.max_hp, g->pl.hp + 15);
            spawn_pickup(g, PK_GRENADE);
            char buf[80];
            snprintf(buf, sizeof(buf), "Vague %d terminee ! +15 PV — la prochaine arrive...", g->wave);
            add_msg(g, buf, 4.0f);
            sfx_play(SFX_WIN);
        }
    }
}

/* ---------------------------------------------------------------- pickups */
static void collect_pickup(Game *g, int idx) {
    Pickup *pk = &g->pickups[idx];
    Player *p = &g->pl;
    char buf[96];
    switch (pk->kind) {
        case PK_HEAL:
            if (p->hp >= p->max_hp) return; /* inutile a pleine sante */
            p->hp = (int)fminf(p->max_hp, p->hp + HEAL_AMOUNT);
            snprintf(buf, sizeof(buf), "+%d PV — citrouille croquee", HEAL_AMOUNT);
            sfx_play(SFX_PICKUP);
            break;
        case PK_SHIELD:
            p->shield = 50; p->shield_t = 25.0f;
            snprintf(buf, sizeof(buf), "Bouclier spectral : +50 d'armure (25 s)");
            sfx_play(SFX_PICKUP);
            break;
        case PK_RAGE:
            p->rage_t = 12.0f;
            snprintf(buf, sizeof(buf), "RAGE ! Degats doubles pendant 12 s");
            sfx_play(SFX_PICKUP);
            break;
        case PK_SWIFT:
            p->swift_t = 15.0f;
            snprintf(buf, sizeof(buf), "Rapidite : +25%% de vitesse pendant 15 s");
            sfx_play(SFX_PICKUP);
            break;
        case PK_GRENADE:
            p->grenades = p->grenades + 2 > 9 ? 9 : p->grenades + 2;
            snprintf(buf, sizeof(buf), "+2 grenades (TRIANGLE pour lancer)");
            sfx_play(SFX_PICKUP);
            break;
        default: /* candle */
            g->frozen_until = g->time + FREEZE_SECONDS;
            snprintf(buf, sizeof(buf), "Bougies sacrees ! Les monstres sont figes");
            sfx_play(SFX_FREEZE);
            break;
    }
    add_msg(g, buf, 2.5f);
    g->items_collected++;
    pk->alive = 0;
    for (int i = 0; i < 10; i++)
        add_particle(g, v3add(pk->pos, v3(0, 0.6f, 0)),
                     v3(grange(g, -2, 2), grange(g, 2, 5), grange(g, -2, 2)),
                     0.5f, 0.15f, PICKUP_COLORS[pk->kind], 1);
}

static void update_pickups(Game *g, float dt) {
    g->power_timer -= dt;
    if (g->power_timer <= 0) {
        g->power_timer = grange(g, 26, 36);
        static const int powers[] = {PK_SHIELD, PK_RAGE, PK_SWIFT, PK_GRENADE};
        int pk = powers[gri(g, 4)];
        if (count_pickups(g, pk) == 0) spawn_pickup(g, pk);
    }
    g->heal_timer -= dt;
    if (g->heal_timer <= 0) {
        if (count_pickups(g, PK_HEAL) < 2) spawn_pickup(g, PK_HEAL);
        g->heal_timer = grange(g, 10, 14);
    }
    g->candle_timer -= dt;
    if (g->candle_timer <= 0) {
        if (count_pickups(g, PK_CANDLE) < 1) spawn_pickup(g, PK_CANDLE);
        g->candle_timer = grange(g, 26, 34);
    }
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup *pk = &g->pickups[i];
        if (!pk->alive) continue;
        pk->t += dt;
        if (v3distxz(pk->pos, g->pl.pos) < PICKUP_RADIUS)
            collect_pickup(g, i);
    }
}

/* ---------------------------------------------------------------- update joueur */
static void update_player(Game *g, const Input *in, float dt) {
    Player *p = &g->pl;
    if (p->dead) return;
    p->invuln_t = fmaxf(p->invuln_t - dt, 0);
    p->hurt_flash = fmaxf(p->hurt_flash - dt * 2.5f, 0);
    p->flash_t = fmaxf(p->flash_t - dt, 0);
    p->rage_t = fmaxf(p->rage_t - dt, 0);
    p->swift_t = fmaxf(p->swift_t - dt, 0);
    if (p->shield_t > 0) { p->shield_t -= dt; if (p->shield_t <= 0) p->shield = 0; }
    p->anim_t += dt;

    /* mouvement relatif a la camera */
    V3 fwd = v3(sinf(g->cam_yaw), 0, cosf(g->cam_yaw));
    V3 right = v3(cosf(g->cam_yaw), 0, -sinf(g->cam_yaw));
    float mx = in->ax, my = in->ay;
    float ml = sqrtf(mx * mx + my * my);
    if (ml > 1) { mx /= ml; my /= ml; }
    V3 wish = v3add(v3scale(right, mx), v3scale(fwd, my));
    int moving = ml > 0.12f;
    if (moving) wish = v3norm(wish);

    /* roulade */
    if (in->roll && !p->rolling) {
        p->rolling = 1;
        p->roll_t = 0.55f;
        p->roll_dir = moving ? wish : v3(sinf(p->yaw), 0, cosf(p->yaw));
        p->invuln_t = fmaxf(p->invuln_t, 0.1f);
        sfx_play(SFX_ROLL);
    }

    float speed = 8.2f; /* course automatique */
    if (p->swift_t > 0) speed *= 1.25f;

    V3 vel;
    if (p->rolling) {
        p->roll_t -= dt;
        p->invuln_t = fmaxf(p->invuln_t, 0.1f);
        vel = v3scale(p->roll_dir, 9.5f);
        if (p->roll_t <= 0) p->rolling = 0;
    } else {
        int aiming = in->shoot;
        float s = speed * (aiming ? 0.62f : 1.0f) * (ml < 1 ? fmaxf(ml, 0.6f) : 1.0f);
        vel = v3scale(wish, s);
    }
    p->pos.x += vel.x * dt;
    p->pos.z += vel.z * dt;
    collide_world(g, &p->pos, 0.38f);

    /* orientation : direction de course, ou cible de visee */
    if (in->shoot) {
        int target = -1; float best_d = SHOT_RANGE;
        V3 f2 = v3(sinf(p->yaw), 0, cosf(p->yaw));
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *e = &g->enemies[i];
            if (!e->alive) continue;
            V3 to = v3sub(e->pos, p->pos);
            float d = v3distxz(p->pos, e->pos);
            if (d > SHOT_RANGE) continue;
            V3 tn = v3norm(v3(to.x, 0, to.z));
            float ang = acosf(clampf(v3dot(tn, f2), -1, 1));
            if (ang < deg2rad(35.0f) && d < best_d) { best_d = d; target = i; }
        }
        V3 face = moving ? wish : fwd;
        if (target >= 0) {
            Enemy *e = &g->enemies[target];
            face = v3norm(v3(e->pos.x - p->pos.x, 0, e->pos.z - p->pos.z));
        }
        float want = atan2f(face.x, face.z);
        float diff = want - p->yaw;
        while (diff > (float)M_PI) diff -= 2 * (float)M_PI;
        while (diff < -(float)M_PI) diff += 2 * (float)M_PI;
        p->yaw += diff * fminf(1.0f, 26.0f * dt);
    } else if (moving) {
        float want = atan2f(wish.x, wish.z);
        float diff = want - p->yaw;
        while (diff > (float)M_PI) diff -= 2 * (float)M_PI;
        while (diff < -(float)M_PI) diff += 2 * (float)M_PI;
        p->yaw += diff * fminf(1.0f, 16.0f * dt);
    }

    /* armes */
    const WDef *w = &WDEFS[p->weapon];
    if (in->wnext || in->wprev) {
        int dir = in->wnext ? 1 : -1;
        int idx = p->weapon;
        for (int k = 0; k < 4; k++) {
            idx = ((idx + dir) % 4 + 4) % 4;
            if (p->unlocked[idx]) break;
        }
        if (idx != p->weapon) {
            p->weapon = idx;
            p->reloading = 0;
            p->mag = WDEFS[idx].mag;
            p->ammo = p->mag;
            sfx_play(SFX_SWITCH);
        }
        w = &WDEFS[p->weapon];
    }
    if (in->grenade) throw_grenade(g);

    p->shoot_cd = fmaxf(p->shoot_cd - dt, 0);
    if (p->reloading) {
        p->reload_t -= dt;
        if (p->reload_t <= 0) {
            p->reloading = 0;
            p->ammo = p->mag;
        }
    } else if (in->reload && p->ammo < p->mag) {
        p->reloading = 1;
        p->reload_t = w->reload;
        sfx_play(SFX_RELOAD);
    } else if (in->shoot && p->shoot_cd <= 0 && !p->rolling) {
        fire_weapon(g);
    }
}

/* ---------------------------------------------------------------- update ennemis */
static void update_enemies(Game *g, float dt) {
    int frozen = g->time < g->frozen_until;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->alive) continue;
        e->attack_cd = fmaxf(e->attack_cd - dt, 0);
        e->hit_flash = fmaxf(e->hit_flash - dt, 0);
        e->phase += dt * (e->hop ? 9.0f : 3.0f);
        if (frozen) continue;

        V3 to_p = v3sub(g->pl.pos, e->pos);
        float dist = v3distxz(e->pos, g->pl.pos);
        V3 dir = dist > 1e-4f ? v3(to_p.x / dist, 0, to_p.z / dist) : v3(0, 0, 0);

        if (!e->stationary) {
            V3 mv = dir;
            if (e->ranged) {
                float pr = ETYPES[e->type].preferred_range;
                if (dist > pr + 1.5f) mv = dir;
                else if (dist < pr - 1.5f) mv = v3scale(dir, -1);
                else mv = v3(0, 0, 0);
            }
            e->pos.x += mv.x * e->speed * dt;
            e->pos.z += mv.z * e->speed * dt;
            collide_world(g, &e->pos, e->radius);
        }
        /* hauteur : sol (sautillement) ou vol stationnaire */
        if (e->hover) {
            e->pos.y = e->base_y + sinf(g->time * 2.0f + e->phase) * 0.15f;
        } else if (e->hop) {
            e->pos.y = 0.15f + fabsf(sinf(e->phase)) * 0.18f;
        } else {
            e->pos.y = 0.15f;
        }

        /* separation entre ennemis */
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (j == i) continue;
            Enemy *o = &g->enemies[j];
            if (!o->alive) continue;
            float dx = e->pos.x - o->pos.x, dz = e->pos.z - o->pos.z;
            float d = sqrtf(dx * dx + dz * dz);
            float min = (e->radius + o->radius) * (e->scale + o->scale) * 0.5f;
            if (d < min && d > 1e-4f) {
                e->pos.x += dx / d * (min - d) * 0.5f;
                e->pos.z += dz / d * (min - d) * 0.5f;
            }
        }

        /* citrouille explosive */
        if (e->explode_radius > 0 && dist <= e->explode_radius) {
            if (dist <= e->explode_radius) game_damage_player(g, e->explode_damage);
            for (int k = 0; k < 20; k++) {
                float a = gr(g) * 6.2831853f, sp = grange(g, 2, 10);
                add_particle(g, v3add(e->pos, v3(0, 0.6f, 0)),
                             v3(cosf(a) * sp, grange(g, 1, 6), sinf(a) * sp),
                             0.5f, 0.3f, RGB(255, 160, 30), 1);
            }
            g->boom_flash = fmaxf(g->boom_flash, 0.7f);
            g->boom_pos = e->pos;
            sfx_play(SFX_EXPLODE);
            kill_enemy(g, i);
            continue;
        }

        /* contact */
        if (dist < e->radius * e->scale + 0.38f + 0.1f && e->attack_cd <= 0 && !g->pl.dead) {
            game_damage_player(g, e->damage);
            e->attack_cd = 1.0f;
        }

        /* tir a distance */
        if (e->ranged && !g->pl.dead) {
            e->shoot_t -= dt;
            if (e->shoot_t <= 0 && dist < 24.0f) {
                e->shoot_t = 2.0f;
                for (int k = 0; k < MAX_PROJ; k++) {
                    if (!g->projs[k].alive) {
                        Projectile *pr = &g->projs[k];
                        pr->alive = 1;
                        V3 from = v3(e->pos.x, e->pos.y + e->height * e->scale * 0.7f, e->pos.z);
                        V3 to = v3(g->pl.pos.x, g->pl.pos.y + 1.1f, g->pl.pos.z);
                        pr->pos = from;
                        pr->vel = v3scale(v3norm(v3sub(to, from)), 11.0f);
                        pr->life = 4.0f;
                        pr->damage = e->damage;
                        pr->color = e->glow;
                        break;
                    }
                }
            }
        }
    }
    /* compacte le tableau */
    int w = 0;
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (g->enemies[i].alive) {
            if (w != i) g->enemies[w] = g->enemies[i];
            w++;
        }
    g->n_enemies = w;
    for (int i = w; i < MAX_ENEMIES; i++) g->enemies[i].alive = 0;
}

void game_debug_kill_all(Game *g) {
    for (int i = 0; i < g->n_enemies; i++)
        if (g->enemies[i].alive) kill_enemy(g, i);
    /* compacte */
    int w = 0;
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (g->enemies[i].alive) {
            if (w != i) g->enemies[w] = g->enemies[i];
            w++;
        }
    g->n_enemies = w;
    for (int i = w; i < MAX_ENEMIES; i++) g->enemies[i].alive = 0;
}

static void update_projectiles(Game *g, float dt) {
    for (int i = 0; i < MAX_PROJ; i++) {
        Projectile *pr = &g->projs[i];
        if (!pr->alive) continue;
        pr->life -= dt;
        pr->pos = v3add(pr->pos, v3scale(pr->vel, dt));
        int hit = 0;
        if (!g->pl.dead) {
            V3 chest = v3(g->pl.pos.x, g->pl.pos.y + 1.0f, g->pl.pos.z);
            if (v3len(v3sub(pr->pos, chest)) < 0.75f) {
                game_damage_player(g, pr->damage);
                hit = 1;
            }
        }
        if (hit || pr->life <= 0 || pr->pos.y < 0.05f) {
            for (int k = 0; k < 4; k++)
                add_particle(g, pr->pos, v3(grange(g, -2, 2), grange(g, 0, 2), grange(g, -2, 2)),
                             0.3f, 0.12f, pr->color, 1);
            pr->alive = 0;
        }
    }
}

static void update_grenades(Game *g, float dt) {
    for (int i = 0; i < MAX_GRENADES; i++) {
        Grenade *gr_ = &g->grenades[i];
        if (!gr_->alive) continue;
        gr_->vel.y -= 20.0f * dt;
        gr_->pos = v3add(gr_->pos, v3scale(gr_->vel, dt));
        if (gr_->pos.y < 0.2f) {
            gr_->pos.y = 0.2f;
            gr_->vel.y = -gr_->vel.y * 0.4f;
            gr_->vel.x *= 0.7f; gr_->vel.z *= 0.7f;
        }
        gr_->fuse -= dt;
        if (gr_->fuse <= 0) {
            explode_grenade(g, gr_);
            gr_->alive = 0;
        }
    }
    int n = 0;
    for (int i = 0; i < MAX_GRENADES; i++) if (g->grenades[i].alive) n++;
    g->n_grenades = n;
}

static void update_particles(Game *g, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &g->parts[i];
        if (!p->alive) continue;
        p->life -= dt;
        if (p->life <= 0) { p->alive = 0; continue; }
        if (p->grav) p->vel.y -= 9.0f * dt;
        p->pos = v3add(p->pos, v3scale(p->vel, dt));
        if (p->pos.y < 0.05f) { p->pos.y = 0.05f; p->vel.y = -p->vel.y * 0.3f; }
    }
    for (int i = 0; i < MAX_TEXTS; i++) {
        FloatText *t = &g->texts[i];
        if (!t->alive) continue;
        t->t += dt;
        t->pos.y += dt * 1.2f;
        if (t->t > 0.7f) t->alive = 0;
    }
    for (int i = 0; i < MAX_MSGS; i++) {
        if (i < g->n_msgs) g->msgs[i].t += dt;
    }
}

/* ---------------------------------------------------------------- camera */
static void update_camera(Game *g, float dt) {
    if (g->state == ST_MENU) {
        g->cam_yaw += 0.22f * dt;
        float d = 7.0f;
        g->cam_pos = v3(g->pl.pos.x - sinf(g->cam_yaw) * d, g->pl.pos.y + 4.0f, g->pl.pos.z - cosf(g->cam_yaw) * d);
        g->cam_look = v3(g->pl.pos.x, g->pl.pos.y + 1.5f, g->pl.pos.z);
        return;
    }
    Player *p = &g->pl;
    /* la camera suit derriere le personnage */
    float diff = p->yaw - g->cam_yaw;
    while (diff > (float)M_PI) diff -= 2 * (float)M_PI;
    while (diff < -(float)M_PI) diff += 2 * (float)M_PI;
    g->cam_yaw += diff * fminf(1.0f, 6.0f * dt);
    float dist = 6.2f, h = 3.6f;
    V3 back = v3(sinf(g->cam_yaw), 0, cosf(g->cam_yaw));
    g->cam_pos = v3(p->pos.x - back.x * dist, p->pos.y + h, p->pos.z - back.z * dist);
    if (g->cam_pos.y < 1.2f) g->cam_pos.y = 1.2f;
    g->cam_look = v3(p->pos.x + back.x * 1.2f, p->pos.y + 1.3f, p->pos.z + back.z * 1.2f);
    g->cam_shake = fmaxf(g->cam_shake - dt * 1.9f, 0);
    g->boom_flash = fmaxf(g->boom_flash - dt * 2.2f, 0);
}

/* ---------------------------------------------------------------- update principal */
void game_update(Game *g, const Input *in, float dt) {
    switch (g->state) {
        case ST_MENU: {
            if (in->up) g->menu_sel = (g->menu_sel + 2) % 3;
            if (in->down) g->menu_sel = (g->menu_sel + 1) % 3;
            if (in->left || in->right) {
                if (g->menu_sel == 2)
                    g->difficulty = (g->difficulty + (in->left ? 2 : 1)) % 3;
            }
            if (in->start || in->shoot) {
                if (g->menu_sel == 0) game_start(g, MODE_NIGHT);
                else if (g->menu_sel == 1) game_start(g, MODE_WAVES);
                else g->difficulty = (g->difficulty + 1) % 3;
                game_save(g);
            }
            update_camera(g, dt);
            update_particles(g, dt);
            return;
        }
        case ST_PAUSE:
            if (in->start) g->state = ST_PLAY;
            else if (in->sel || in->reload) { g->state = ST_MENU; }
            else if (in->shoot) { game_start(g, g->mode); }
            return;
        case ST_WON:
        case ST_LOST: {
            if (g->state == ST_LOST && !g->end_shown) {
                g->end_timer -= dt;
                if (g->end_timer <= 0) g->end_shown = 1;
            }
            if (g->end_shown) {
                if (in->start || in->shoot) {
                    game_start(g, g->mode);
                } else if (in->sel && g->state == ST_WON && g->mode == MODE_NIGHT) {
                    /* mode infini apres l'aube */
                    g->endless = 1;
                    g->state = ST_PLAY;
                    g->end_shown = 0;
                    g->spawn_timer = 1.0f;
                    add_msg(g, "MODE INFINI — Tiens le plus longtemps possible !", 4.0f);
                    sfx_play(SFX_BELL);
                } else if (in->sel || in->reload) {
                    g->state = ST_MENU;
                }
            }
            update_particles(g, dt);
            update_camera(g, dt);
            return;
        }
        case ST_PLAY:
            break;
    }

    if (in->start) { g->state = ST_PAUSE; return; }

    g->time += dt;
    float f = clampf(g->time / NIGHT_DURATION, 0, 1);

    update_player(g, in, dt);
    if (g->pl.dead) { update_enemies(g, dt); update_projectiles(g, dt); update_grenades(g, dt); update_particles(g, dt); update_camera(g, dt); return; }

    if (g->mode == MODE_WAVES && !g->endless) {
        update_waves(g, dt);
        update_pickups(g, dt);
        if (g->state != ST_PLAY) { update_particles(g, dt); update_camera(g, dt); return; }
    } else {
        /* hordes de citrouilles explosives */
        if (!g->endless) {
            for (int hi = 0; hi < 2; hi++) {
                float hf = hi == 0 ? 0.3f : 0.65f;
                if (f >= hf && !g->hordes_done[hi]) {
                    g->hordes_done[hi] = 1;
                    add_msg(g, "VAGUE DE CITROUILLES EXPLOSIVES !", 3.5f);
                    sfx_play(SFX_BELL);
                    for (int i = 0; i < 6; i++) {
                        float a = 6.2831853f * i / 6.0f;
                        V3 p = v3(g->pl.pos.x + cosf(a) * 11.0f, 0.15f, g->pl.pos.z + sinf(a) * 11.0f);
                        collide_world(g, &p, 0.45f);
                        game_spawn_enemy(g, ET_BOMBER, p);
                    }
                }
            }
            /* boss de fin de nuit */
            if (!g->boss_spawned && f >= 0.88f && !g->endless) {
                g->boss_spawned = 1;
                V3 p = game_random_open_point(g, g->pl.pos, 15.0f, 1.2f);
                game_spawn_enemy(g, ET_BOSS, p);
                add_msg(g, "LE ROI CITROUILLE SE LEVE !", 4.0f);
                sfx_play(SFX_BELL);
                g->cam_shake = 0.7f;
            }
        }
        /* apparition des ennemis */
        g->spawn_timer -= dt;
        if (g->spawn_timer <= 0) {
            if (g->n_enemies < max_enemies(g, f)) spawn_enemy_at_random(g, f);
            static const float iv[] = {1.35f, 1.0f, 0.8f};
            g->spawn_timer = lerpf(3.2f, 0.8f, f) * diff_mult(g, iv);
        }
        update_pickups(g, dt);

        /* messages de la nuit */
        if (g->msg_index < 4 && f >= MSG_FRAC[g->msg_index]) {
            add_msg(g, MSG_TEXT[g->msg_index], 4.0f);
            sfx_play(SFX_BELL);
            g->msg_index++;
        }

        if (g->time >= NIGHT_DURATION && !g->endless) {
            /* victoire : l'aube se leve */
            g->state = ST_WON;
            g->end_shown = 1;
            g->new_best = g->best_time < NIGHT_DURATION;
            if (!g->ach[4]) g->ach[4] = 1;
            g->nights_won++;
            g->best_time = NIGHT_DURATION;
            g->n_enemies = 0;
            for (int i = 0; i < MAX_ENEMIES; i++) g->enemies[i].alive = 0;
            game_save(g);
            sfx_play(SFX_WIN);
        }
    }

    update_enemies(g, dt);
    update_projectiles(g, dt);
    update_grenades(g, dt);
    update_particles(g, dt);
    update_camera(g, dt);
}
