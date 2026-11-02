/* Couche graphique bas niveau : GU, textures procedurales, maillages, texte bitmap. */
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspkernel.h>

#include "gfx.h"
#include "font_data.h"

#define BUF_WIDTH 512

static unsigned int __attribute__((aligned(16))) g_list[524288 / 4];
static void *fbp0 = (void *)0;
static void *fbp1 = (void *)0x88000;
static void *fzb  = (void *)0x110000;

#define FLAGS_TEX (GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D)
#define FLAGS_COL (GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D)

/* ---------------------------------------------------------------- textures */
static unsigned int __attribute__((aligned(16))) tex_data[TEX_COUNT][64 * 64];
static int tex_w[TEX_COUNT], tex_h[TEX_COUNT];

static void set_px(int slot, int x, int y, unsigned int c) {
    if (x < 0 || y < 0 || x >= tex_w[slot] || y >= tex_h[slot]) return;
    tex_data[slot][y * tex_w[slot] + x] = c;
}

static void gen_pumpkin(int slot, unsigned int body, unsigned int dark) {
    int S = 64, cx = 32, cy = 34, r = 26;
    tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            float dx = (x - cx) / (float)r, dy = (y - cy) / (float)(r * 0.88f);
            float d = dx * dx + dy * dy;
            if (d > 1.0f) { set_px(slot, x, y, 0); continue; }
            float rib = 0.85f + 0.15f * fabsf(sinf(atan2f(dy, dx) * 3.0f));
            unsigned int col = RGB((unsigned)(((body) & 0xFF) * rib),
                                   (unsigned)((((body >> 8) & 0xFF)) * rib),
                                   (unsigned)((((body >> 16) & 0xFF)) * rib));
            if (x >= cx - 3 && x <= cx + 3 && y >= 4 && y <= 12)
                col = RGB(70, 120, 45);
            int face = 0;
            if (y >= 24 && y <= 32) {
                if ((x >= 20 && x <= 27) || (x >= 37 && x <= 44)) {
                    float ex = (x - (x < 32 ? 23.5f : 40.5f)) / 4.0f, ey = (y - 28) / 4.0f;
                    if (ex * ex + ey * ey < 1.0f) face = 1;
                }
            }
            if (y >= 40 && y <= 50 && x >= 18 && x <= 46) {
                float my = (y - 45) / 5.0f;
                if (fabsf((x - 32) / 14.0f) + fabsf(my) * 0.7f < 1.0f) face = 1;
            }
            if (face) col = dark;
            set_px(slot, x, y, col);
        }
}

static void gen_skull(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            float dx = (x - 32) / 22.0f, dy = (y - 28) / 24.0f;
            float d = dx * dx + dy * dy;
            if (d > 1.0f) { set_px(slot, x, y, 0); continue; }
            unsigned int col = RGB(225, 225, 235);
            if (d > 0.55f) col = RGB(190, 190, 205);
            if (y > 42 && y < 54 && x > 22 && x < 42 && d <= 1.0f) col = RGB(210, 210, 220);
            if ((x - 24) * (x - 24) + (y - 26) * (y - 26) < 36 ||
                (x - 40) * (x - 40) + (y - 26) * (y - 26) < 36) col = RGB(20, 20, 30);
            if ((x - 32) * (x - 32) + (y - 36) * (y - 36) < 8) col = RGB(20, 20, 30);
            if (y >= 46 && y <= 52 && ((x - 26) % 6 < 2) && x > 24 && x < 40) col = RGB(40, 40, 55);
            set_px(slot, x, y, col);
        }
}

static void gen_ghost(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            float dx = (x - 32) / 20.0f, dy = (y - 30) / 26.0f;
            float d = dx * dx + dy * dy;
            int in = d <= 1.0f;
            if (in && y > 48) {
                if (sinf(x * 1.4f) > 0.2f) in = 0;
            }
            if (!in) { set_px(slot, x, y, 0); continue; }
            unsigned int a = d > 0.8f ? 140 : 230;
            unsigned int col = RGBA(235, 240, 255, a);
            if ((x - 25) * (x - 25) + (y - 26) * (y - 26) < 14 ||
                (x - 39) * (x - 39) + (y - 26) * (y - 26) < 14) col = RGBA(30, 30, 50, 255);
            if ((x - 32) * (x - 32) + (y - 40) * (y - 40) < 6) col = RGBA(30, 30, 50, 255);
            set_px(slot, x, y, col);
        }
}

static void gen_totem(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            unsigned int col = 0;
            if (x >= 24 && x <= 40 && y >= 8 && y <= 60) {
                col = RGB(90, 55, 130);
                if ((x - 32) * (x - 32) / 36.0f + (y - 22) * (y - 22) / 64.0f < 1.0f && y < 34)
                    col = RGB(220, 220, 235);
                if (y < 34 && ((x - 28) * (x - 28) + (y - 20) * (y - 20) < 6 ||
                               (x - 36) * (x - 36) + (y - 20) * (y - 20) < 6))
                    col = RGB(20, 10, 30);
                if (x == 24 || x == 40 || y == 8) col = RGB(50, 30, 80);
            }
            set_px(slot, x, y, col);
        }
}

static void gen_player(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            unsigned int col = 0;
            if (x >= 22 && x <= 42 && y >= 26 && y <= 58 && fabsf((float)(x - 32)) < 11 - (y - 26) * 0.12f)
                col = RGB(50, 70, 140);
            float dx = (x - 32) / 11.0f, dy = (y - 17) / 12.0f;
            if (dx * dx + dy * dy <= 1.0f) col = RGB(240, 200, 160);
            if (dx * dx + dy * dy <= 1.0f && (dy < -0.35f || fabsf(dx) > 0.75f)) col = RGB(70, 45, 120);
            if ((x - 28) * (x - 28) + (y - 18) * (y - 18) < 2.5f ||
                (x - 36) * (x - 36) + (y - 18) * (y - 18) < 2.5f) col = RGB(25, 25, 40);
            if (x >= 40 && x <= 50 && y >= 38 && y <= 43) col = RGB(60, 60, 70);
            if (x >= 48 && x <= 52 && y >= 39 && y <= 42) col = RGB(30, 30, 40);
            set_px(slot, x, y, col);
        }
}

static unsigned int hash_noise(int x, int y) {
    unsigned int h = x * 374761393u + y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (h ^ (h >> 16)) & 0xFF;
}

static void gen_ground(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            unsigned int n = hash_noise(x, y);
            unsigned int n2 = hash_noise(x / 4, y / 4);
            unsigned int v = 90 + (n >> 2) + (n2 >> 1);
            if (v > 200) v = 200;
            set_px(slot, x, y, RGB(v, v * 0.92f, v * 0.8f));
        }
}

static void gen_stone(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            unsigned int n = hash_noise(x, y);
            unsigned int v = 120 + (n >> 3);
            int mortar = (y % 16 < 2) || ((x + (y / 16) * 21) % 32 < 2);
            if (mortar) v = 70;
            set_px(slot, x, y, RGB(v, v, v + 8));
        }
}

static void gen_fence(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            unsigned int n = hash_noise(x, y);
            int picket = (x % 16) < 9;
            int rail = (y % 32) < 5 || (y % 32) > 26;
            unsigned int v = picket ? 95 : 70;
            v += (n >> 4);
            unsigned int col = rail || picket ? RGB(v, v * 0.75f, v * 0.5f) : RGB(15, 12, 10);
            set_px(slot, x, y, col);
        }
}

static void gen_wood(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            unsigned int n = hash_noise(x / 2, y);
            int plank = (x % 21) < 2;
            unsigned int v = 110 + (n >> 3);
            if (plank) v = 60;
            set_px(slot, x, y, RGB(v, v * 0.68f, v * 0.42f));
        }
}

static void gen_glow(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            float dx = (x - 32) / 32.0f, dy = (y - 32) / 32.0f;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 1) d = 1;
            unsigned int a = (unsigned int)(powf(1.0f - d, 2.0f) * 255);
            set_px(slot, x, y, RGBA(255, 255, 255, a));
        }
}

static void gen_moon(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            float dx = (x - 32) / 30.0f, dy = (y - 32) / 30.0f;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 1) { set_px(slot, x, y, 0); continue; }
            float base = 0.95f - d * 0.2f;
            unsigned int col = RGB((unsigned)(base * 235), (unsigned)(base * 240), (unsigned)(base * 255));
            if ((x - 24) * (x - 24) + (y - 26) * (y - 26) < 12) col = RGB(180, 188, 215);
            if ((x - 40) * (x - 40) + (y - 40) * (y - 40) < 9) col = RGB(185, 192, 218);
            if ((x - 34) * (x - 34) + (y - 18) * (y - 18) < 6) col = RGB(190, 196, 222);
            set_px(slot, x, y, col);
        }
}

static void gen_tree(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            unsigned int col = 0;
            if (x >= 29 && x <= 35 && y >= 30 && y <= 62)
                col = RGB(70 + (hash_noise(x, y) >> 4), 52, 36);
            float dx = (x - 32) / 24.0f, dy = (y - 22) / 20.0f;
            float d = dx * dx + dy * dy * 1.3f;
            if (d < 1.0f && y < 46) {
                unsigned int n = hash_noise(x * 3, y * 3);
                unsigned int v = 34 + (n >> 2);
                col = RGB(v, v * 0.85f, v * 0.6f);
                if (n > 200) col = RGB(v + 30, v + 20, v);
            }
            if (y >= 30 && y <= 44 && abs(x - 32) < (44 - y) * 0.7f && abs(x - 32) > 2)
                col = RGB(60, 46, 32);
            set_px(slot, x, y, col);
        }
}

static void gen_grenade(int slot) {
    int S = 64; tex_w[slot] = tex_h[slot] = S;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            float dx = (x - 32) / 22.0f, dy = (y - 34) / 22.0f;
            float d = sqrtf(dx * dx + dy * dy);
            if (d > 1) { set_px(slot, x, y, 0); continue; }
            unsigned int v = 60 + (unsigned)((1 - d) * 60);
            unsigned int col = RGB(v / 2, v, v / 2);
            if ((x - 26) * (x - 26) + (y - 26) * (y - 26) < 16) col = RGB(200, 220, 210);
            set_px(slot, x, y, col);
        }
}

/* ---------------------------------------------------------------- maillages */
static Vtx mesh_box[36];
static Vtx mesh_cyl[8 * 6 + 8 * 3];
static Vtx mesh_cone[8 * 6];
static int mesh_count[MESH_COUNT];

static void build_meshes(void) {
    static const float p[8][3] = {
        {-0.5f, 0, -0.5f}, {0.5f, 0, -0.5f}, {0.5f, 1, -0.5f}, {-0.5f, 1, -0.5f},
        {-0.5f, 0, 0.5f}, {0.5f, 0, 0.5f}, {0.5f, 1, 0.5f}, {-0.5f, 1, 0.5f}
    };
    static const int faces[6][4] = {
        {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7}, {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0}
    };
    int vi = 0;
    for (int f = 0; f < 6; f++) {
        const int *q = faces[f];
        const float uv[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
        int tris[2][3] = {{q[0], q[1], q[2]}, {q[0], q[2], q[3]}};
        for (int t = 0; t < 2; t++)
            for (int k = 0; k < 3; k++) {
                int i = tris[t][k];
                mesh_box[vi].u = uv[k][0]; mesh_box[vi].v = uv[k][1];
                mesh_box[vi].color = 0xFFFFFFFF;
                mesh_box[vi].x = p[i][0]; mesh_box[vi].y = p[i][1]; mesh_box[vi].z = p[i][2];
                vi++;
            }
    }
    mesh_count[MESH_BOX] = vi;

    int SEG = 8;
    vi = 0;
    for (int s = 0; s < SEG; s++) {
        float a0 = 6.2831853f * s / SEG, a1 = 6.2831853f * (s + 1) / SEG;
        float x0 = cosf(a0) * 0.5f, z0 = sinf(a0) * 0.5f;
        float x1 = cosf(a1) * 0.5f, z1 = sinf(a1) * 0.5f;
        Vtx quad[6] = {
            {0, 1, 0xFFFFFFFF, x0, 0, z0}, {1, 1, 0xFFFFFFFF, x1, 0, z1}, {1, 0, 0xFFFFFFFF, x1, 1, z1},
            {0, 1, 0xFFFFFFFF, x0, 0, z0}, {1, 0, 0xFFFFFFFF, x1, 1, z1}, {0, 0, 0xFFFFFFFF, x0, 1, z0}
        };
        for (int k = 0; k < 6; k++) mesh_cyl[vi++] = quad[k];
    }
    for (int s = 0; s < SEG; s++) {
        float a0 = 6.2831853f * s / SEG, a1 = 6.2831853f * (s + 1) / SEG;
        Vtx tri[3] = {
            {0.5f, 0.5f, 0xFFFFFFFF, 0, 1, 0},
            {0, 0, 0xFFFFFFFF, cosf(a0) * 0.5f, 1, sinf(a0) * 0.5f},
            {1, 1, 0xFFFFFFFF, cosf(a1) * 0.5f, 1, sinf(a1) * 0.5f}
        };
        for (int k = 0; k < 3; k++) mesh_cyl[vi++] = tri[k];
    }
    mesh_count[MESH_CYL] = vi;

    vi = 0;
    for (int s = 0; s < SEG; s++) {
        float a0 = 6.2831853f * s / SEG, a1 = 6.2831853f * (s + 1) / SEG;
        Vtx tri[3] = {
            {0, 1, 0xFFFFFFFF, cosf(a0) * 0.5f, 0, sinf(a0) * 0.5f},
            {1, 1, 0xFFFFFFFF, cosf(a1) * 0.5f, 0, sinf(a1) * 0.5f},
            {0.5f, 0, 0xFFFFFFFF, 0, 1, 0}
        };
        for (int k = 0; k < 3; k++) mesh_cone[vi++] = tri[k];
    }
    mesh_count[MESH_CONE] = vi;
}

/* ---------------------------------------------------------------- etat interne */
static V3 cam_right, cam_up, cam_fwd;
static M4 gfx_proj, gfx_view;
#define FOV 60.0f

void gfx_init(void) {
    tex_w[TEX_FONT] = tex_h[TEX_FONT] = 256;

    gen_pumpkin(TEX_PUMPKIN, RGB(255, 122, 24), RGB(40, 15, 5));
    gen_skull(TEX_SKULL);
    gen_ghost(TEX_GHOST);
    gen_totem(TEX_TOTEM);
    gen_player(TEX_PLAYER);
    gen_ground(TEX_GROUND);
    gen_stone(TEX_STONE);
    gen_fence(TEX_FENCE);
    gen_wood(TEX_WOOD);
    gen_glow(TEX_GLOW);
    gen_moon(TEX_MOON);
    gen_grenade(TEX_GRENADE);
    gen_tree(TEX_TREE);
    build_meshes();

    sceGuInit();
    sceGuStart(GU_DIRECT, g_list);
    sceGuDrawBuffer(GU_PSM_8888, fbp0, BUF_WIDTH);
    sceGuDispBuffer(SCR_W, SCR_H, fbp1, BUF_WIDTH);
    sceGuDepthBuffer(fzb, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_W / 2), 2048 - (SCR_H / 2));
    sceGuViewport(2048, 2048, SCR_W, SCR_H);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, SCR_W, SCR_H);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_SMOOTH);
    sceGuDisable(GU_CULL_FACE);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_CLIP_PLANES);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuColor(0xFFFFFFFF);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplaySetMode(0, SCR_W, SCR_H);
    sceDisplaySetFrameBuf(fbp1, BUF_WIDTH, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTFRAME);
    sceGuDisplay(GU_TRUE);
    sceDisplaySetFrameBuf((void *)(0x04000000 + 0x88000), BUF_WIDTH,
                      PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTFRAME);
}

void gfx_begin_frame(unsigned int clear_color) {
    sceGuStart(GU_DIRECT, g_list);
    sceGuClearColor(clear_color);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDisable(GU_BLEND);
    sceGuDisable(GU_FOG);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexScale(1.0f, 1.0f);
    sceGuTexOffset(0.0f, 0.0f);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
}

void gfx_end_frame(void) {
    sceKernelDcacheWritebackAll();
    sceGuFinish();
    sceGuSync(0, 0);
    void *disp = sceGuSwapBuffers();
    sceDisplaySetFrameBuf((void *)(0x04000000 + (unsigned int)disp),
                          BUF_WIDTH, PSP_DISPLAY_PIXEL_FORMAT_8888,
                          PSP_DISPLAY_SETBUF_NEXTFRAME);
}

void gfx_begin_3d(const V3 *cam_pos, const V3 *cam_look) {
    V3 up = v3(0, 1, 0);
    gfx_proj = m4perspective(FOV, (float)SCR_W / SCR_H, 0.3f, 400.0f);
    gfx_view = m4lookat(*cam_pos, *cam_look, up);
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(FOV, (float)SCR_W / SCR_H, 0.3f, 400.0f);
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumLookAt((V3 *)cam_pos, (V3 *)cam_look, &up);
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    cam_fwd = v3norm(v3sub(*cam_look, *cam_pos));
    cam_right = v3norm(v3cross(cam_fwd, up));
    cam_up = v3cross(cam_right, cam_fwd);
}

void gfx_begin_2d(void) {
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumOrtho(0, SCR_W, SCR_H, 0, -1, 1);
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
}

void gfx_depth(int on) {
    if (on) sceGuEnable(GU_DEPTH_TEST);
    else sceGuDisable(GU_DEPTH_TEST);
}

void gfx_blend_alpha(void) {
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
}

void gfx_blend_add(void) {
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFF);
}

void gfx_blend_off(void) {
    sceGuDisable(GU_BLEND);
}

void gfx_fog(int on, float near, float far, unsigned int color) {
    if (on) {
        sceGuEnable(GU_FOG);
        sceGuFog(near, far, color);
    } else {
        sceGuDisable(GU_FOG);
    }
}

void gfx_tex(int slot, int wrap_repeat) {
    sceGuEnable(GU_TEXTURE_2D);
    if (slot == TEX_FONT)
        sceGuTexImage(0, 256, 256, 256, FONT_ATLAS);
    else
        sceGuTexImage(0, tex_w[slot], tex_h[slot], tex_w[slot], tex_data[slot]);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    if (wrap_repeat) sceGuTexWrap(GU_REPEAT, GU_REPEAT);
    else sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexScale(1.0f, 1.0f);
    sceGuTexOffset(0.0f, 0.0f);
}

void gfx_no_tex(void) {
    sceGuDisable(GU_TEXTURE_2D);
}

/* ---------------------------------------------------------------- dessin */
void gfx_mesh(int mesh, int tex, unsigned int color) {
    gfx_tex(tex, 0);
    Vtx *v = mesh == MESH_BOX ? mesh_box : (mesh == MESH_CYL ? mesh_cyl : mesh_cone);
    int n = mesh_count[mesh];
    Vtx *tmp = (Vtx *)sceGuGetMemory(sizeof(Vtx) * n);
    if (!tmp) return;
    for (int i = 0; i < n; i++) {
        tmp[i] = v[i];
        tmp[i].color = color;
    }
    sceGuDrawArray(GU_TRIANGLES, FLAGS_TEX, n, 0, tmp);
}

void gfx_box(float x, float y, float z, float yaw, float sx, float sy, float sz, int tex, unsigned int color) {
    sceGumPushMatrix();
    V3 t = {x, y, z};
    sceGumTranslate(&t);
    sceGumRotateY(yaw);
    V3 s = {sx, sy, sz};
    sceGumScale(&s);
    gfx_mesh(MESH_BOX, tex, color);
    sceGumPopMatrix();
}

void gfx_billboard_uv(const V3 *c, float w, float h, int tex, float u0, float v0, float u1, float v1, unsigned int color) {
    gfx_tex(tex, 0);
    V3 r = v3scale(cam_right, w * 0.5f), u = v3scale(cam_up, h * 0.5f);
    Vtx *v = (Vtx *)sceGuGetMemory(sizeof(Vtx) * 4);
    if (!v) return;
    v[0] = (Vtx){u0, v1, color, c->x - r.x - u.x, c->y - r.y - u.y, c->z - r.z - u.z};
    v[1] = (Vtx){u1, v1, color, c->x + r.x - u.x, c->y + r.y - u.y, c->z + r.z - u.z};
    v[2] = (Vtx){u0, v0, color, c->x - r.x + u.x, c->y - r.y + u.y, c->z - r.z + u.z};
    v[3] = (Vtx){u1, v0, color, c->x + r.x + u.x, c->y + r.y + u.y, c->z + r.z + u.z};
    sceGuDrawArray(GU_TRIANGLE_STRIP, FLAGS_TEX, 4, 0, v);
}

void gfx_billboard(const V3 *c, float w, float h, int tex, unsigned int color) {
    gfx_billboard_uv(c, w, h, tex, 0, 0, 1, 1, color);
}

void gfx_ground_quad(float x, float z, float w, float d, float y, int tex, unsigned int color, float uvrep) {
    gfx_tex(tex, uvrep > 1.0f);
    float hw = w * 0.5f, hd = d * 0.5f;
    Vtx *v = (Vtx *)sceGuGetMemory(sizeof(Vtx) * 4);
    if (!v) return;
    v[0] = (Vtx){0, uvrep, color, x - hw, y, z - hd};
    v[1] = (Vtx){uvrep, uvrep, color, x + hw, y, z - hd};
    v[2] = (Vtx){0, 0, color, x - hw, y, z + hd};
    v[3] = (Vtx){uvrep, 0, color, x + hw, y, z + hd};
    sceGuDrawArray(GU_TRIANGLE_STRIP, FLAGS_TEX, 4, 0, v);
}

/* ---------------------------------------------------------------- texte */
static int utf8_next(const char **s) {
    unsigned char c = (unsigned char)**s;
    if (c < 0x80) {
        (*s)++;
        return c;
    }
    if ((c & 0xE0) == 0xC0 && (*s)[1]) {
        int r = ((c & 0x1F) << 6) | ((*s)[1] & 0x3F);
        *s += 2;
        return r;
    }
    if ((c & 0xF0) == 0xE0 && (*s)[1] && (*s)[2]) {
        int r = ((c & 0x0F) << 12) | (((*s)[1] & 0x3F) << 6) | ((*s)[2] & 0x3F);
        *s += 3;
        return r;
    }
    (*s)++;
    return '?';
}

static int cp_count(const char *s) {
    int n = 0;
    while (*s) {
        utf8_next(&s);
        n++;
    }
    return n;
}

int gfx_text_width(const char *s, float scale) {
    return (int)(cp_count(s) * 9 * scale);
}

void gfx_text(float x, float y, float scale, unsigned int color, const char *s) {
    gfx_tex(TEX_FONT, 0);
    sceGuEnable(GU_TEXTURE_2D);
    float adv = 9.0f * scale, size = 16.0f * scale;
    float cx = x;
    while (*s) {
        int cp = utf8_next(&s);
        if (cp < FONT_FIRST) cp = FONT_FIRST;
        if (cp > FONT_FIRST + FONT_COUNT - 1) cp = '?';
        int idx = cp - FONT_FIRST;
        float u0 = (idx % 16) / 16.0f, v0 = (idx / 16) / 16.0f;
        float u1 = u0 + 1.0f / 16.0f, v1 = v0 + 1.0f / 16.0f;
        Vtx *v = (Vtx *)sceGuGetMemory(sizeof(Vtx) * 4);
        if (!v) return;
        v[0] = (Vtx){u0, v1, color, cx, y + size, 0};
        v[1] = (Vtx){u1, v1, color, cx + size, y + size, 0};
        v[2] = (Vtx){u0, v0, color, cx, y, 0};
        v[3] = (Vtx){u1, v0, color, cx + size, y, 0};
        sceGuDrawArray(GU_TRIANGLE_STRIP, FLAGS_TEX, 4, 0, v);
        cx += adv;
    }
}

void gfx_text_center(float cx, float y, float scale, unsigned int color, const char *s) {
    gfx_text(cx - gfx_text_width(s, scale) * 0.5f, y, scale, color, s);
}

void gfx_rect(float x, float y, float w, float h, unsigned int color) {
    gfx_no_tex();
    CVtx *v = (CVtx *)sceGuGetMemory(sizeof(CVtx) * 4);
    if (!v) return;
    v[0] = (CVtx){color, x, y + h, 0};
    v[1] = (CVtx){color, x + w, y + h, 0};
    v[2] = (CVtx){color, x, y, 0};
    v[3] = (CVtx){color, x + w, y, 0};
    sceGuDrawArray(GU_TRIANGLE_STRIP, FLAGS_COL, 4, 0, v);
}

void gfx_text3d(const V3 *pos, float scale, unsigned int color, const char *s) {
    gfx_tex(TEX_FONT, 0);
    float adv = 9.0f * scale, size = 16.0f * scale;
    V3 start = v3add(*pos, v3scale(cam_right, -gfx_text_width(s, 1.0f) * 0.5f * scale));
    float cx = 0;
    while (*s) {
        int cp = utf8_next(&s);
        if (cp < FONT_FIRST) cp = FONT_FIRST;
        if (cp > FONT_FIRST + FONT_COUNT - 1) cp = '?';
        int idx = cp - FONT_FIRST;
        float u0 = (idx % 16) / 16.0f, v0 = (idx / 16) / 16.0f;
        float u1 = u0 + 1.0f / 16.0f, v1 = v0 + 1.0f / 16.0f;
        V3 c = v3add(start, v3scale(cam_right, cx + adv * 0.5f));
        V3 r = v3scale(cam_right, size * 0.5f), u = v3scale(cam_up, size * 0.5f);
        Vtx *v = (Vtx *)sceGuGetMemory(sizeof(Vtx) * 4);
        if (!v) return;
        v[0] = (Vtx){u0, v1, color, c.x - r.x - u.x, c.y - r.y - u.y, c.z - r.z - u.z};
        v[1] = (Vtx){u1, v1, color, c.x + r.x - u.x, c.y + r.y - u.y, c.z + r.z - u.z};
        v[2] = (Vtx){u0, v0, color, c.x - r.x + u.x, c.y - r.y + u.y, c.z - r.z + u.z};
        v[3] = (Vtx){u1, v0, color, c.x + r.x + u.x, c.y + r.y + u.y, c.z + r.z + u.z};
        sceGuDrawArray(GU_TRIANGLE_STRIP, FLAGS_TEX, 4, 0, v);
        cx += adv;
    }
}

int gfx_project(const V3 *world, float *sx, float *sy) {
    float clip[4];
    M4 m = m4mul(gfx_proj, gfx_view);
    m4transform(m, *world, clip);
    if (clip[3] <= 0.001f) return 0;
    *sx = (clip[0] / clip[3] * 0.5f + 0.5f) * SCR_W;
    *sy = (1.0f - (clip[1] / clip[3] * 0.5f + 0.5f)) * SCR_H;
    return 1;
}
