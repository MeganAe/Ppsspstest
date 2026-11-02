#ifndef GFX_H
#define GFX_H

#include "util.h"
#include "game.h"

#define SCR_W 480
#define SCR_H 272

/* sommets GU */
typedef struct { float u, v; unsigned int color; float x, y, z; } Vtx;
typedef struct { unsigned int color; float x, y, z; } CVtx;

/* textures procedurales */
enum {
    TEX_FONT = 0, TEX_PUMPKIN, TEX_SKULL, TEX_GHOST, TEX_TOTEM, TEX_PLAYER,
    TEX_GROUND, TEX_STONE, TEX_FENCE, TEX_WOOD, TEX_GLOW, TEX_MOON, TEX_GRENADE, TEX_TREE,
    TEX_COUNT
};

/* maillages */
enum { MESH_BOX = 0, MESH_CYL, MESH_CONE, MESH_COUNT };

void gfx_init(void);

void gfx_begin_frame(unsigned int clear_color);
void gfx_end_frame(void);

void gfx_begin_3d(const V3 *cam_pos, const V3 *cam_look);
void gfx_begin_2d(void);

void gfx_depth(int on);
void gfx_blend_alpha(void);
void gfx_blend_add(void);
void gfx_blend_off(void);
void gfx_fog(int on, float near, float far, unsigned int color);

void gfx_tex(int slot, int wrap_repeat);
void gfx_no_tex(void);

void gfx_box(float x, float y, float z, float yaw, float sx, float sy, float sz, int tex, unsigned int color);
void gfx_mesh(int mesh, int tex, unsigned int color);   /* dessine le maillage courant (matrice modele deja posee) */
void gfx_billboard(const V3 *c, float w, float h, int tex, unsigned int color);
void gfx_billboard_uv(const V3 *c, float w, float h, int tex, float u0, float v0, float u1, float v1, unsigned int color);
void gfx_ground_quad(float x, float z, float w, float d, float y, int tex, unsigned int color, float uvrep);

/* texte 2D (apres gfx_begin_2d) */
int  gfx_text_width(const char *s, float scale);
void gfx_text(float x, float y, float scale, unsigned int color, const char *s);
void gfx_text_center(float cx, float y, float scale, unsigned int color, const char *s);
void gfx_rect(float x, float y, float w, float h, unsigned int color);

/* texte 3D en billboard (couleur de la camera) */
void gfx_text3d(const V3 *pos, float scale, unsigned int color, const char *s);

/* projection monde -> ecran (matrices de la frame 3D courante) */
int gfx_project(const V3 *world, float *sx, float *sy);

#endif
