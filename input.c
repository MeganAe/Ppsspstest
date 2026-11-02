#include <pspctrl.h>
#include "input.h"
#include "util.h"

static unsigned int prev_buttons = 0;

void input_init(void) {
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    prev_buttons = 0;
}

void input_poll(Input *out) {
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);
    unsigned int b = pad.Buttons;
    unsigned int pressed = b & ~prev_buttons;
    prev_buttons = b;

    memset(out, 0, sizeof(*out));

    /* stick analogique gauche : mouvement (-1..1), avec zone morte */
    float ax = (float)pad.Lx - 128.0f;
    float ay = (float)pad.Ly - 128.0f;
    if (fabsf(ax) < 22) ax = 0; else ax = ax / 128.0f;
    if (fabsf(ay) < 22) ay = 0; else ay = ay / 128.0f;
    /* croix directionnelle en secours */
    if (b & PSP_CTRL_LEFT) ax = -1;
    if (b & PSP_CTRL_RIGHT) ax = 1;
    if (b & PSP_CTRL_UP) ay = -1;
    if (b & PSP_CTRL_DOWN) ay = 1;
    out->ax = clampf(ax, -1, 1);
    out->ay = clampf(-ay, -1, 1);   /* pousser le stick vers le haut = avancer */

    out->shoot = (b & PSP_CTRL_CROSS) != 0;
    out->reload = (pressed & PSP_CTRL_SQUARE) != 0;
    out->roll = (pressed & PSP_CTRL_CIRCLE) != 0;
    out->grenade = (pressed & PSP_CTRL_TRIANGLE) != 0;
    out->wnext = (pressed & PSP_CTRL_RTRIGGER) != 0;
    out->wprev = (pressed & PSP_CTRL_LTRIGGER) != 0;
    out->start = (pressed & PSP_CTRL_START) != 0;
    out->sel = (pressed & PSP_CTRL_SELECT) != 0;

    out->up = (pressed & PSP_CTRL_UP) != 0;
    out->down = (pressed & PSP_CTRL_DOWN) != 0;
    out->left = (pressed & PSP_CTRL_LEFT) != 0;
    out->right = (pressed & PSP_CTRL_RIGHT) != 0;
}
