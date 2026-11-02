/* Rendu d'une frame : ciel nuit->aube, cimetiere 3D, entites, HUD, menus. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pspgu.h>
#include <pspgum.h>

#include "render.h"
#include "gfx.h"
#include "audio.h"

/* palettes (float RGB 0..1), reprises de game.gd */
static const float C_NIGHT_TOP[3]     = {0.02f, 0.03f, 0.10f};
static const float C_NIGHT_HORIZON[3] = {0.08f, 0.10f, 0.22f};
static const float C_DAWN_TOP[3]      = {0.35f, 0.45f, 0.75f};
static const float C_DAWN_HORIZON[3]  = {1.00f, 0.62f, 0.38f};
static const float C_NIGHT_FOG[3]     = {0.07f, 0.09f, 0.18f};
static const float C_DAWN_FOG[3]      = {0.85f, 0.60f, 0.50f};
static const float C_NIGHT_AMB[3]     = {0.24f, 0.29f, 0.48f};
static const float C_DAWN_AMB[3]      = {0.65f, 0.52f, 0.52f};
static const float C_MOON[3]          = {0.55f, 0.65f, 1.00f};
static const float C_SUN[3]           = {1.00f, 0.80f, 0.60f};

static unsigned int fcol(const float *c) {
    return RGB((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255));
}
static unsigned int fcol_lerp(const float *a, const float *b, float t) {
    float c[3] = {lerpf(a[0], b[0], t), lerpf(a[1], b[1], t), lerpf(a[2], b[2], t)};
    return fcol(c);
}
static unsigned int col_lerp(unsigned int a, unsigned int b, float t) {
    int ar = a & 0xFF, ag = (a >> 8) & 0xFF, ab = (a >> 16) & 0xFF, aa = (a >> 24) & 0xFF;
    int br = b & 0xFF, bg = (b >> 8) & 0xFF, bb = (b >> 16) & 0xFF, ba = (b >> 24) & 0xFF;
    return RGBA(ar + (int)((br - ar) * t), ag + (int)((bg - ag) * t),
                ab + (int)((bb - ab) * t), aa + (int)((ba - aa) * t));
}
static unsigned int with_alpha(unsigned int c, int a) {
    return (c & 0x00FFFFFF) | ((unsigned int)(a & 0xFF) << 24);
}

/* ---------------------------------------------------------------- ciel */
static CVtx sky_mesh[24 * 6];
static V3 star_dir[80];
static int sky_built = 0;

static void build_sky(void) {
    if (sky_built) return;
    sky_built = 1;
    int SEG = 24, vi = 0;
    float R = 170.0f, y0 = -10.0f, y1 = 110.0f;
    for (int s = 0; s < SEG; s++) {
        float a0 = 6.2831853f * s / SEG, a1 = 6.2831853f * (s + 1) / SEG;
        CVtx q[6] = {
            {0, cosf(a0) * R, y0, sinf(a0) * R},
            {0, cosf(a1) * R, y0, sinf(a1) * R},
            {0, cosf(a0) * R, y1, sinf(a0) * R},
            {0, cosf(a1) * R, y0, sinf(a1) * R},
            {0, cosf(a1) * R, y1, sinf(a1) * R},
            {0, cosf(a0) * R, y1, sinf(a0) * R}
        };
        for (int k = 0; k < 6; k++) sky_mesh[vi++] = q[k];
    }
    srand(777);
    for (int i = 0; i < 80; i++) {
        float a = randf() * 6.2831853f;
        float y = 0.08f + randf() * 0.9f;
        float r = sqrtf(1 - y * y);
        star_dir[i] = v3(cosf(a) * r, y, sinf(a) * r);
    }
}

static void draw_sky(Game *g, float d, unsigned int sky_top,
                     unsigned int horizon, const V3 *cam) {
    build_sky();

    gfx_depth(0);
    gfx_blend_off();
    gfx_fog(0, 0, 0, 0);
    gfx_no_tex();

    sceGumPushMatrix();

    V3 t = {cam->x, 0, cam->z};
    sceGumTranslate(&t);

    CVtx *tmp = (CVtx *)sceGuGetMemory(sizeof(CVtx) * (24 * 6));

    if (tmp) {
        for (int i = 0; i < 24 * 6; i++) {
            tmp[i] = sky_mesh[i];
            tmp[i].color = (sky_mesh[i].y < 50.0f)
                ? horizon : sky_top;
        }

        sceGuDrawArray(
            GU_TRIANGLES,
            GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
            24 * 6,
            0,
            tmp
        );
    }

    sceGumPopMatrix();

    int sa = (int)((1.0f - d) * 210);

    if (sa > 10) {
        gfx_blend_add();

        for (int i = 0; i < 80; i++) {
            V3 p = v3(
                cam->x + star_dir[i].x * 160,
                cam->y * 0.5f + star_dir[i].y * 160 + 10,
                cam->z + star_dir[i].z * 160
            );

            gfx_billboard(
                &p, 0.7f, 0.7f,
                TEX_GLOW,
                RGBA(200, 215, 255, sa)
            );
        }

        gfx_blend_off();
    }

    V3 mdir = v3norm(v3(0.30f, 0.42f, -0.86f));

    V3 mp = v3(
        cam->x + mdir.x * 150,
        cam->y + mdir.y * 150,
        cam->z + mdir.z * 150
    );

    unsigned int mcol = fcol_lerp(C_MOON, C_SUN, d);

    gfx_blend_add();

    gfx_billboard(
        &mp,
        24.0f + 8.0f * d,
        24.0f + 8.0f * d,
        TEX_MOON,
        with_alpha(mcol, 235)
    );

    gfx_billboard(
        &mp,
        46.0f,
        46.0f,
        TEX_GLOW,
        with_alpha(mcol, (int)(60 * (1.0f - d * 0.5f)))
    );

    gfx_blend_off();
    gfx_depth(1);
}

    /* lune -> soleil */
    V3 mdir = v3norm(v3(0.30f, 0.42f, -0.86f));
    V3 mp = v3(cam->x + mdir.x * 150, cam->y + mdir.y * 150, cam->z + mdir.z * 150);
    unsigned int mcol = fcol_lerp(C_MOON, C_SUN, d);
    gfx_blend_add();
    gfx_billboard(&mp, 24.0f + 8.0f * d, 24.0f + 8.0f * d, TEX_MOON, with_alpha(mcol, 235));
    gfx_billboard(&mp, 46.0f, 46.0f, TEX_GLOW, with_alpha(mcol, (int)(60 * (1.0f - d * 0.5f))));
    gfx_blend_off();
    gfx_depth(1);
}

/* ---------------------------------------------------------------- decor */
static int enemy_tex(int t) {
    switch (t) {
        case ET_SKULL: case ET_RIBCAGE: return TEX_SKULL;
        case ET_GHOST: return TEX_GHOST;
        case ET_TOTEM: return TEX_TOTEM;
        default: return TEX_PUMPKIN;
    }
}

static void draw_world(Game *g, unsigned int amb, const V3 *cam) {
    /* sol */
    gfx_fog(1, 20.0f, 160.0f, fcol_lerp(C_NIGHT_FOG, C_DAWN_FOG, smoothstep(0.6f, 1.0f, clampf(g->time / NIGHT_DURATION, 0, 1))));
    gfx_blend_off();
    gfx_ground_quad(0, 0, 400, 400, 0.0f, TEX_GROUND, with_alpha(amb, 255), 90.0f);

    /* cloture + portail + crypte */
    unsigned int stone = col_lerp(amb, 0xFFFFFFFF, 0.25f);
    gfx_box(0, 0, -ARENA, 0, ARENA * 2 + 4, 2.4f, 0.5f, TEX_FENCE, amb);
    gfx_box(0, 0, ARENA, 0, ARENA * 2 + 4, 2.4f, 0.5f, TEX_FENCE, amb);
    gfx_box(-ARENA, 0, 0, 1.5707963f, ARENA * 2 + 4, 2.4f, 0.5f, TEX_FENCE, amb);
    gfx_box(ARENA, 0, 0, 1.5707963f, ARENA * 2 + 4, 2.4f, 0.5f, TEX_FENCE, amb);
    gfx_box(-1.45f, 0, ARENA - 0.3f, 0, 1.1f, 3.6f, 1.0f, TEX_STONE, stone);
    gfx_box(1.45f, 0, ARENA - 0.3f, 0, 1.1f, 3.6f, 1.0f, TEX_STONE, stone);
    gfx_box(0, 3.6f, ARENA - 0.3f, 0, 4.6f, 0.9f, 1.0f, TEX_STONE, stone);
    gfx_box(0, 0, -ARENA + 8.0f, 0, 9.0f, 4.5f, 7.0f, TEX_STONE, stone);
    gfx_box(0, 4.5f, -ARENA + 8.0f, 0, 9.6f, 0.8f, 7.6f, TEX_STONE, stone);

    /* decor */
    V3 lights[40]; float light_h[40]; int n_lights = 0;
    for (int i = 0; i < g->n_decor; i++) {
        Decor *dc = &g->decor[i];
        float s = dc->scale, x = dc->x, z = dc->z;
        switch (dc->kind) {
            case DC_GRAVE:      gfx_box(x, 0, z, dc->yaw, 1.2f * s, 1.0f * s, 0.4f * s, TEX_STONE, amb); break;
            case DC_GRAVESTONE: gfx_box(x, 0, z, dc->yaw, 0.9f * s, 1.3f * s, 0.35f * s, TEX_STONE, amb); break;
            case DC_MARKER:     gfx_box(x, 0, z, dc->yaw, 0.7f * s, 0.7f * s, 0.3f * s, TEX_STONE, amb); break;
            case DC_COFFIN:     gfx_box(x, 0, z, dc->yaw, 1.8f * s, 0.7f * s, 0.8f * s, TEX_WOOD, amb); break;
            case DC_TREE: {
                /* arbre en billboard (economique en polygones) */
                unsigned int tint = col_lerp(amb, 0xFFFFFFFF, 0.25f);
                V3 tc = v3(x, 2.3f * s, z);
                gfx_billboard(&tc, 2.6f * s, 4.8f * s, TEX_TREE, with_alpha(tint, 255));
                break;
            }
            case DC_LANTERN:
                gfx_box(x, 0, z, dc->yaw, 0.24f * s, 2.4f * s, 0.24f * s, TEX_WOOD, col_lerp(amb, RGB(40, 30, 22), 0.5f));
                gfx_box(x, 2.4f * s, z, dc->yaw, 0.45f * s, 0.55f * s, 0.45f * s, TEX_WOOD, col_lerp(amb, RGB(255, 190, 110), 0.35f));
                if (n_lights < 40) { lights[n_lights] = v3(x, 0, z); light_h[n_lights] = 2.6f * s; n_lights++; }
                break;
            case DC_CANDLE:
                gfx_box(x, 0, z, dc->yaw, 0.22f * s, 0.5f * s, 0.22f * s, TEX_STONE, amb);
                if (n_lights < 40) { lights[n_lights] = v3(x, 0, z); light_h[n_lights] = 0.8f * s; n_lights++; }
                break;
            case DC_SHRINE:
                gfx_box(x, 0, z, dc->yaw, 2.2f * s, 1.8f * s, 1.2f * s, TEX_STONE, amb);
                if (n_lights < 40) { lights[n_lights] = v3(x, 0, z); light_h[n_lights] = 2.2f * s; n_lights++; }
                break;
            case DC_PUMPKIN:
                gfx_billboard(&(V3){x, 0.42f * s, z}, 0.85f * s, 0.85f * s, TEX_PUMPKIN, col_lerp(amb, 0xFFFFFFFF, 0.4f));
                break;
            case DC_BONE:
                gfx_box(x, 0.02f, z, dc->yaw, 0.9f * s, 0.08f, 0.25f * s, TEX_STONE, col_lerp(amb, RGB(230, 230, 220), 0.5f));
                break;
            case DC_POST:
                gfx_box(x, 0, z, dc->yaw, 0.5f * s, 2.6f * s, 0.5f * s, TEX_WOOD, amb);
                break;
            case DC_BENCH:
                gfx_box(x, 0, z, dc->yaw, 2.0f * s, 0.5f * s, 0.7f * s, TEX_WOOD, amb);
                break;
            case DC_FENCE_BROKEN:
                gfx_box(x, 0, z, dc->yaw, 1.6f * s, 1.0f * s, 0.3f * s, TEX_FENCE, amb);
                break;
        }
    }

    /* halos de lumiere au sol + lueur des lanternes */
    gfx_blend_add();
    for (int i = 0; i < n_lights; i++) {
        gfx_ground_quad(lights[i].x, lights[i].z, 7.0f, 7.0f, 0.03f, TEX_GLOW, RGBA(255, 170, 80, 50), 1.0f);
        V3 hp = v3(lights[i].x, light_h[i], lights[i].z);
        gfx_billboard(&hp, 1.6f, 1.6f, TEX_GLOW, RGBA(255, 185, 95, 120));
    }
    /* lanterne du joueur */
    gfx_ground_quad(g->pl.pos.x, g->pl.pos.z, 10.0f, 10.0f, 0.03f, TEX_GLOW, RGBA(255, 190, 110, 42), 1.0f);
    V3 plp = v3(g->pl.pos.x, g->pl.pos.y + 2.3f, g->pl.pos.z);
    gfx_billboard(&plp, 2.2f, 2.2f, TEX_GLOW, RGBA(255, 200, 130, 70));
    gfx_blend_alpha();
}

/* ---------------------------------------------------------------- entites */
static void draw_shadow(Game *g, float x, float z, float r) {
    gfx_ground_quad(x, z, r * 2.4f, r * 2.4f, 0.02f, TEX_GLOW, RGBA(0, 0, 0, 110), 1.0f);
}

static void draw_enemy(Game *g, Enemy *e) {
    float pulse = 1.0f + 0.05f * sinf(g->time * 6.0f + e->phase);
    float w = fmaxf(e->radius * 2.2f, e->height * 0.9f) * e->scale * pulse;
    float h = e->height * 1.15f * e->scale * pulse;
    V3 c = v3(e->pos.x, e->pos.y + e->height * e->scale * 0.55f, e->pos.z);
    unsigned int col = e->glow;
    if (e->hit_flash > 0) col = col_lerp(col, 0xFFFFFFFF, e->hit_flash / 0.12f);
    gfx_billboard(&c, w, h, enemy_tex(e->type), with_alpha(col, 255));
    /* barre de vie */
    if (e->hp < e->max_hp) {
        float frac = clampf(e->hp / e->max_hp, 0, 1);
        float y = e->pos.y + e->height * e->scale + 0.35f;
        V3 bc = v3(e->pos.x, y, e->pos.z);
        float bw = (e->boss || e->elite) ? 1.6f : 1.0f;
        gfx_billboard(&bc, bw, 0.12f, TEX_GLOW, RGBA(10, 10, 14, 190));
        unsigned int fc = frac < 0.33f ? RGB(242, 89, 64) : (frac < 0.66f ? RGB(250, 204, 77) : RGB(140, 242, 115));
        V3 fc3 = v3(e->pos.x - bw * 0.5f * (1 - frac), y, e->pos.z);
        gfx_billboard(&fc3, bw * frac, 0.12f, TEX_GLOW, with_alpha(fc, 230));
    }
}

static void draw_entities(Game *g, float d) {
    /* ombres */
    gfx_blend_alpha();
    draw_shadow(g, g->pl.pos.x, g->pl.pos.z, 0.5f);
    for (int i = 0; i < g->n_enemies; i++)
        draw_shadow(g, g->enemies[i].pos.x, g->enemies[i].pos.z, g->enemies[i].radius * g->enemies[i].scale);

    /* ennemis, du plus loin au plus pres */
    int order[MAX_ENEMIES];
    for (int i = 0; i < g->n_enemies; i++) order[i] = i;
    for (int i = 0; i < g->n_enemies; i++)
        for (int j = i + 1; j < g->n_enemies; j++) {
            float di = v3distxz(g->enemies[order[i]].pos, g->cam_pos);
            float dj = v3distxz(g->enemies[order[j]].pos, g->cam_pos);
            if (dj > di) { int t = order[i]; order[i] = order[j]; order[j] = t; }
        }
    for (int i = 0; i < g->n_enemies; i++)
        draw_enemy(g, &g->enemies[order[i]]);

    /* joueur */
    Player *p = &g->pl;
    float ph = p->rolling ? 0.55f : 1.0f;
    unsigned int pcol = p->invuln_t > 0 ? RGBA(255, 255, 255, 150) : 0xFFFFFFFF;
    V3 pc = v3(p->pos.x, p->pos.y + 0.85f, p->pos.z);
    gfx_billboard(&pc, 1.1f, 1.7f * ph, TEX_PLAYER, pcol);
    if (p->shield > 0) {
        gfx_blend_add();
        gfx_billboard(&pc, 2.6f, 2.6f, TEX_GLOW, RGBA(90, 160, 255, 90));
        gfx_blend_alpha();
    }
    /* flash de tir */
    if (p->flash_t > 0) {
        V3 fwd = v3(sinf(p->yaw), 0, cosf(p->yaw));
        V3 m = v3(p->pos.x + fwd.x * 0.9f, p->pos.y + 1.3f, p->pos.z + fwd.z * 0.9f);
        gfx_blend_add();
        gfx_billboard(&m, 0.9f, 0.9f, TEX_GLOW, RGBA(255, 225, 130, 210));
        gfx_blend_alpha();
    }

    /* pickups : rayon + icone */
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup *pk = &g->pickups[i];
        if (!pk->alive) continue;
        float bob = sinf(pk->t * 2.4f) * 0.12f;
        unsigned int col = PICKUP_COLORS[pk->kind];
        gfx_blend_add();
        V3 beam = v3(pk->pos.x, pk->pos.y + 2.0f, pk->pos.z);
        gfx_billboard(&beam, 0.35f, 4.0f, TEX_GLOW, with_alpha(col, 55));
        V3 ic = v3(pk->pos.x, pk->pos.y + 0.55f + bob, pk->pos.z);
        int tex = (pk->kind == PK_HEAL || pk->kind == PK_SHIELD) ? TEX_PUMPKIN : (pk->kind == PK_GRENADE ? TEX_GRENADE : TEX_GLOW);
        gfx_billboard(&ic, 0.75f, 0.75f, tex, with_alpha(col, 255));
        gfx_billboard(&ic, 1.6f, 1.6f, TEX_GLOW, with_alpha(col, 90));
        gfx_blend_alpha();
    }

    /* projectiles ennemis */
    gfx_blend_add();
    for (int i = 0; i < MAX_PROJ; i++) {
        Projectile *pr = &g->projs[i];
        if (!pr->alive) continue;
        gfx_billboard(&pr->pos, 0.5f, 0.5f, TEX_GLOW, with_alpha(pr->color, 255));
    }
    /* grenades */
    for (int i = 0; i < MAX_GRENADES; i++) {
        Grenade *gr = &g->grenades[i];
        if (!gr->alive) continue;
        gfx_billboard(&gr->pos, 0.5f, 0.5f, TEX_GRENADE, 0xFFFFFFFF);
    }
    /* particules */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *pa = &g->parts[i];
        if (!pa->alive) continue;
        float frac = pa->life / pa->max_life;
        int a = (int)(frac * 255);
        gfx_billboard(&pa->pos, pa->size * (0.5f + 0.5f * frac) * 2, pa->size * (0.5f + 0.5f * frac) * 2,
                      TEX_GLOW, with_alpha(pa->color, a));
    }
    /* explosion */
    if (g->boom_flash > 0.01f) {
        V3 bp = v3(g->boom_pos.x, g->boom_pos.y + 1.0f, g->boom_pos.z);
        gfx_billboard(&bp, 7.0f * (1.2f - g->boom_flash * 0.2f), 7.0f * (1.2f - g->boom_flash * 0.2f),
                      TEX_GLOW, RGBA(255, 170, 70, (int)(g->boom_flash * 220)));
    }
    gfx_blend_off();

    /* chiffres de degats en 3D */
    gfx_blend_alpha();
    for (int i = 0; i < MAX_TEXTS; i++) {
        FloatText *t = &g->texts[i];
        if (!t->alive) continue;
        float a = 1.0f - t->t / 0.7f;
        unsigned int col = t->crit ? RGBA(255, 110, 40, (int)(a * 255)) : RGBA(255, 240, 200, (int)(a * 255));
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", t->amount);
        gfx_text3d(&t->pos, t->crit ? 0.075f : 0.055f, col, buf);
    }
}

/* ---------------------------------------------------------------- HUD */
static void fmt_time(char *buf, int size, float s) {
    int t = (int)s;
    if (t < 0) t = 0;
    snprintf(buf, size, "%02d:%02d", t / 60, t % 60);
}

static void draw_bar(float x, float y, float w, float h, float frac, unsigned int color) {
    gfx_rect(x, y, w, h, RGBA(8, 8, 12, 200));
    if (frac > 0) gfx_rect(x + 1, y + 1, (w - 2) * frac, h - 2, color);
    gfx_rect(x, y, w, 1, RGBA(255, 255, 255, 60));
    gfx_rect(x, y + h - 1, w, 1, RGBA(0, 0, 0, 160));
}

static void clock_text(Game *g, char *buf, int size) {
    float f = clampf(g->time / NIGHT_DURATION, 0, 1);
    if (g->mode == MODE_WAVES && !g->endless) {
        if (g->wave_rest_t > 0)
            snprintf(buf, size, "REPOS %d s", (int)(g->wave_rest_t + 0.99f));
        else
            snprintf(buf, size, "VAGUE %d/10 — %d", g->wave, g->wave_to_spawn + g->n_enemies);
    } else if (g->endless) {
        char t[16];
        fmt_time(t, sizeof(t), g->time - NIGHT_DURATION);
        snprintf(buf, size, "06:00 +%s", t);
    } else {
        int minutes = (int)(f * 360);
        snprintf(buf, size, "%02d:%02d", minutes / 60, minutes % 60);
    }
}

static void draw_hud(Game *g) {
    char buf[96];
    Player *p = &g->pl;
    float f = clampf(g->time / NIGHT_DURATION, 0, 1);

    /* barre XP + niveau */
    draw_bar(0, 0, SCR_W, 6, (float)g->xp / g->xp_next, RGB(90, 200, 250));
    snprintf(buf, sizeof(buf), "NV %d", g->level);
    gfx_text(6, 8, 1.0f, RGB(180, 220, 255), buf);

    /* horloge */
    clock_text(g, buf, sizeof(buf));
    gfx_text_center(SCR_W / 2, 6, 2.0f, RGB(235, 240, 255), buf);
    /* progression de la nuit */
    draw_bar(SCR_W / 2 - 100, 42, 200, 4, f, RGB(255, 160, 60));

    /* barre de vie + bouclier */
    float hpf = (float)p->hp / p->max_hp;
    unsigned int hcol = hpf < 0.3f ? RGB(230, 60, 50) : (hpf < 0.6f ? RGB(240, 170, 50) : RGB(90, 210, 90));
    if (p->shield > 0) draw_bar(10, 12, 150, 5, (float)p->shield / 50.0f, RGB(90, 150, 255));
    draw_bar(10, 20, 150, 12, hpf, hcol);
    snprintf(buf, sizeof(buf), "%d", p->hp);
    gfx_text(14, 21, 0.75f, RGB(255, 255, 255), buf);

    /* tues */
    snprintf(buf, sizeof(buf), "TUES %d", g->kills);
    gfx_text(SCR_W - gfx_text_width(buf, 1.0f) - 8, 10, 1.0f, RGB(255, 200, 120), buf);

    /* boss */
    for (int i = 0; i < g->n_enemies; i++) {
        Enemy *e = &g->enemies[i];
        if (e->alive && e->boss) {
            draw_bar(SCR_W / 2 - 150, 50, 300, 8, e->hp / e->max_hp, RGB(255, 70, 30));
            gfx_text_center(SCR_W / 2, 60, 0.8f, RGB(255, 150, 90), "LE ROI CITROUILLE");
            break;
        }
    }

    /* arme + munitions + grenades */
    const WDef *w = &WDEFS[p->weapon];
    snprintf(buf, sizeof(buf), "%s", w->name);
    gfx_text(SCR_W - gfx_text_width(buf, 1.0f) - 8, SCR_H - 46, 1.0f, RGB(220, 225, 240), buf);
    if (p->reloading)
        snprintf(buf, sizeof(buf), "RECHARGE...");
    else
        snprintf(buf, sizeof(buf), "%d / %d", p->ammo, p->mag);
    gfx_text(SCR_W - gfx_text_width(buf, 1.4f) - 8, SCR_H - 30, 1.4f, RGB(255, 220, 130), buf);
    snprintf(buf, sizeof(buf), "GRENADES x%d", p->grenades);
    gfx_text(SCR_W - gfx_text_width(buf, 1.0f) - 8, SCR_H - 14, 1.0f, RGB(160, 255, 160), buf);

    /* pouvoirs actifs */
    float y = SCR_H - 16;
    if (p->rage_t > 0) { snprintf(buf, sizeof(buf), "RAGE %.0fs", p->rage_t); gfx_text(8, y, 1.0f, RGB(255, 90, 60), buf); y -= 15; }
    if (p->swift_t > 0) { snprintf(buf, sizeof(buf), "VITE %.0fs", p->swift_t); gfx_text(8, y, 1.0f, RGB(120, 255, 150), buf); y -= 15; }
    if (p->shield_t > 0) { snprintf(buf, sizeof(buf), "BOUCLIER %.0fs", p->shield_t); gfx_text(8, y, 1.0f, RGB(110, 170, 255), buf); y -= 15; }
    if (g->time < g->frozen_until) {
        snprintf(buf, sizeof(buf), "FIGE %.0fs", g->frozen_until - g->time);
        gfx_text(8, y, 1.0f, RGB(150, 210, 255), buf);
    }

    /* visee */
    if (g->state == ST_PLAY) {
        gfx_rect(SCR_W / 2 - 1, SCR_H / 2 - 6, 2, 12, RGBA(255, 255, 255, 200));
        gfx_rect(SCR_W / 2 - 6, SCR_H / 2 - 1, 12, 2, RGBA(255, 255, 255, 200));
    }

    /* messages centraux */
    for (int i = 0; i < g->n_msgs; i++) {
        Msg *m = &g->msgs[i];
        float a = m->t < 0.3f ? m->t / 0.3f : (m->dur - m->t) / 0.5f;
        if (a < 0) a = 0;
        if (a > 1) a = 1;
        gfx_text_center(SCR_W / 2, 78 + i * 20, 1.1f, RGBA(255, 235, 190, (int)(a * 255)), m->text);
    }

    /* vignette de degats */
    if (p->hurt_flash > 0)
        gfx_rect(0, 0, SCR_W, SCR_H, RGBA(255, 20, 10, (int)(p->hurt_flash * 80)));
    if (g->boom_flash > 0.01f)
        gfx_rect(0, 0, SCR_W, SCR_H, RGBA(255, 200, 120, (int)(g->boom_flash * 100)));
}

/* ---------------------------------------------------------------- ecrans */
static const char *DIFF_NAMES[3] = {"HISTOIRE", "NORMAL", "CAUCHEMAR"};

static void draw_menu(Game *g) {
    gfx_rect(0, 0, SCR_W, SCR_H, RGBA(4, 6, 16, 170));
    gfx_text_center(SCR_W / 2, 34, 2.6f, RGB(255, 145, 40), "LA NUIT DES CITROUILLES");
    gfx_text_center(SCR_W / 2, 84, 1.1f, RGB(170, 190, 240), "EDITION PSP — PPSSPP");

    const char *items[3] = {"SURVIVRE JUSQU'A L'AUBE", "MODE VAGUES (10 VAGUES)", NULL};
    char diffbuf[64];
    snprintf(diffbuf, sizeof(diffbuf), "DIFFICULTE : %s  (< >)", DIFF_NAMES[g->difficulty]);
    for (int i = 0; i < 3; i++) {
        const char *txt = i == 2 ? diffbuf : items[i];
        unsigned int col = i == g->menu_sel ? RGB(255, 170, 60) : RGB(190, 195, 215);
        char line[80];
        snprintf(line, sizeof(line), "%s%s", i == g->menu_sel ? "> " : "  ", txt);
        gfx_text_center(SCR_W / 2, 130 + i * 26, 1.3f, col, line);
    }

    char buf[96];
    if (g->best_time > 0) {
        char t[16];
        fmt_time(t, sizeof(t), g->best_time);
        snprintf(buf, sizeof(buf), "Record : %s   Nuits survécues : %d   Monstres : %d", t, g->nights_won, g->total_kills);
    } else {
        snprintf(buf, sizeof(buf), "Aucune partie jouee — bonne chance...");
    }
    gfx_text_center(SCR_W / 2, 220, 1.0f, RGB(150, 160, 190), buf);
    gfx_text_center(SCR_W / 2, 244, 0.85f, RGB(130, 140, 170),
                    "STICK: bouger  CROIX: tirer  CARRE: recharger  ROND: roulade  TRIANGLE: grenade  L/R: arme  START: pause");
}

static void draw_stats(Game *g, float y) {
    char buf[96], t[16];
    int acc = g->shots_fired > 0 ? (int)(100 * g->shots_hit / g->shots_fired) : 0;
    fmt_time(t, sizeof(t), g->time);
    snprintf(buf, sizeof(buf), "Temps : %s   Tueurs : %d   Niveau : %d", t, g->kills, g->level);
    gfx_text_center(SCR_W / 2, y, 1.1f, RGB(210, 215, 235), buf);
    snprintf(buf, sizeof(buf), "Precision : %d %%   Objets : %d", acc, g->items_collected);
    gfx_text_center(SCR_W / 2, y + 20, 1.1f, RGB(210, 215, 235), buf);
    if (g->new_best) {
        gfx_text_center(SCR_W / 2, y + 40, 1.1f, RGB(255, 220, 90), "NOUVEAU RECORD !");
    }
}

static void draw_end(Game *g) {
    gfx_rect(0, 0, SCR_W, SCR_H, RGBA(4, 6, 16, 180));
    if (g->state == ST_WON) {
        gfx_text_center(SCR_W / 2, 60, 2.4f, RGB(255, 210, 110), "L'AUBE SE LEVE !");
        if (g->mode == MODE_WAVES)
            gfx_text_center(SCR_W / 2, 108, 1.2f, RGB(220, 225, 240), "Tu as vaincu les 10 vagues !");
        else
            gfx_text_center(SCR_W / 2, 108, 1.2f, RGB(220, 225, 240), "Tu as tenu jusqu'au bout de la nuit !");
        draw_stats(g, 140);
        if (g->end_shown) {
            if (g->mode == MODE_NIGHT)
                gfx_text_center(SCR_W / 2, 210, 1.0f, RGB(255, 180, 90), "[CROIX] Rejouer    [SELECT] Mode infini    [CARRE] Menu");
            else
                gfx_text_center(SCR_W / 2, 210, 1.0f, RGB(255, 180, 90), "[CROIX] Rejouer    [CARRE] Menu");
        }
    } else {
        gfx_text_center(SCR_W / 2, 60, 2.4f, RGB(255, 80, 60), "LES TENEBRES T'ONT AVALE...");
        if (g->mode == MODE_WAVES)
            gfx_text_center(SCR_W / 2, 108, 1.2f, RGB(220, 225, 240), "Tu es tombe a la vague...");
        else {
            char t[16];
            fmt_time(t, sizeof(t), NIGHT_DURATION - g->time);
            char buf[64];
            snprintf(buf, sizeof(buf), "Encore %s avant l'aube.", t);
            gfx_text_center(SCR_W / 2, 108, 1.2f, RGB(220, 225, 240), buf);
        }
        draw_stats(g, 140);
        if (g->end_shown)
            gfx_text_center(SCR_W / 2, 210, 1.0f, RGB(255, 180, 90), "[CROIX] Rejouer    [CARRE] Menu");
    }
}

static void draw_pause(Game *g) {
    gfx_rect(0, 0, SCR_W, SCR_H, RGBA(4, 6, 16, 170));
    gfx_text_center(SCR_W / 2, 80, 2.2f, RGB(230, 235, 250), "PAUSE");
    char buf[96];
    clock_text(g, buf, sizeof(buf));
    char line[128];
    snprintf(line, sizeof(line), "Il est %s  —  %d monstres vaincus", buf, g->kills);
    gfx_text_center(SCR_W / 2, 130, 1.1f, RGB(200, 205, 225), line);
    gfx_text_center(SCR_W / 2, 170, 1.0f, RGB(255, 180, 90), "[START] Reprendre    [CROIX] Recommencer    [CARRE] Menu");
}

/* ---------------------------------------------------------------- frame */
void render_frame(Game *g) {
    float f = clampf(g->time / NIGHT_DURATION, 0, 1);
    if (g->state == ST_MENU) f = 0.0f;
    float d = smoothstep(0.6f, 1.0f, f);
    music_set_dawn(d);

    unsigned int horizon = fcol_lerp(C_NIGHT_HORIZON, C_DAWN_HORIZON, d);
    unsigned int sky_top = fcol_lerp(C_NIGHT_TOP, C_DAWN_TOP, d);
    unsigned int amb = fcol_lerp(C_NIGHT_AMB, C_DAWN_AMB, d);

    gfx_begin_frame(horizon);

    /* camera + secousse */
    V3 cam_pos = g->cam_pos, cam_look = g->cam_look;
    if (g->cam_shake > 0.001f) {
        cam_pos.x += (randf() - 0.5f) * 0.35f * g->cam_shake;
        cam_pos.y += (randf() - 0.5f) * 0.25f * g->cam_shake;
        cam_look.x += (randf() - 0.5f) * 0.3f * g->cam_shake;
    }

    gfx_begin_3d(&cam_pos, &cam_look);
    draw_sky(g, d, sky_top, horizon, &cam_pos);
    draw_world(g, amb, &cam_pos);
    if (g->state != ST_MENU || 1)
        draw_entities(g, d);

    /* HUD 2D */
    gfx_begin_2d();
    gfx_depth(0);
    gfx_blend_alpha();
    if (g->state == ST_PLAY || g->state == ST_PAUSE) draw_hud(g);
    if (g->state == ST_MENU) draw_menu(g);
    else if (g->state == ST_PAUSE) draw_pause(g);
    else if (g->state == ST_WON || g->state == ST_LOST) { draw_hud(g); draw_end(g); }

    gfx_end_frame();
}

void render_init(void) {
    gfx_init();
}
