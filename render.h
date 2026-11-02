#ifndef RENDER_H
#define RENDER_H

#include "game.h"

/* Initialise le GU (une fois) : textures procedurales, maillages, framebuffers. */
void render_init(void);
/* Dessine une frame complete (monde 3D + HUD) et swap les buffers. */
void render_frame(Game *g);

#endif
