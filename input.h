#ifndef INPUT_H
#define INPUT_H

#include "game.h"

void input_init(void);
/* Lit la manette et remplit la structure Input du jeu. */
void input_poll(Input *out);

#endif
